#include <stdarg.h>
#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "mem_internals.h"
#include "mem.h"
#include "util.h"

void debug_block(struct block_header* b, const char* fmt, ... );
//void debug(const char* fmt, ... );

extern inline block_size size_from_capacity( block_capacity cap );
extern inline block_capacity capacity_from_size( block_size sz );


extern inline bool region_is_invalid(const struct region *r);

/**
 * Метод вычисляющий, сколько страниц может уместиться в памяти
 * @param mem Кол-во доступной памяти
 * @return Кол-во страниц
 */
static size_t pages_count(block_size query_size) {
    return query_size.bytes / getpagesize() + ((query_size.bytes % getpagesize()) > 0);
}

/**
 * Метод вычисляющий кол-во памяти которое мы можем mapping страничками
 * @param mem Кол-во доступной памяти
 * @return Кол-во памяти
 */
static size_t round_pages(block_size query_size) { return getpagesize() * pages_count(query_size); }

/**
 * Метод инициализации региона памяти
 * @param addr Адрес начала региона
 * @param length Размер региона
 * @return Возвращается готовую структуру
 */
static struct region region_init(void *restrict addr, block_size region_size) {
    return (struct region) {.addr = addr, .size = region_size.bytes, .extends = false};
}

/**
 * Метод инициализации блока региона
 * @param addr Адрес начала блока
 * @param block_sz  Размер блока
 * @param next  Ссылка на следующий блок региона
 * @return Кастит указатель к block_header* и кладет в него одноименную структуру
 */
static void block_init(void *restrict addr, const block_capacity block_capacity, void *restrict next) {

    /* Скастили указатель к типу struct block_header* */
    struct block_header *restrict const block_header_ptr = (struct block_header *) addr;

    /* Положили в указываемую область памяти структуру struct block_header */
    *block_header_ptr = (struct block_header) {
            .next = next,
            .capacity = block_capacity,
            .is_free = true
    };
}

/**
 * Актуальный размер для региона памяти
 * <p>
 * (Выбираем между REGION_MIN_SIZE и запрашиваемым размером)
 * @param query Запрашиваемый размер региона
 * @return Актуальный размер региона на основе сравнения
 */
static block_size region_actual_size(const block_size query_size) {
    return size_max(round_pages(query_size), REGION_MIN_SIZE);
}

/**
 * Метод маппинга виртуальной памяти с помощью syscall mmap
 * @param addr Адрес начала региона
 * @param length Размер региона
 * @param additional_flags Доп флаги для syscall
 * @return Возвращает указатель на начало региона
 */
static void *map_pages(void const *const addr, const size_t length, const int additional_flags) {
    return mmap((void *) addr,
                length,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | additional_flags,
                0,
                0);
}

/**
 * Метод для аллоцирования региона и инициализации его блоком
 * @param addr Адрес с которого желательно, чтобы начинался регион
 * @param query Запрашиваемый размер региона
 * @return Возвращаем ссылку на регион с актуальным размером
 */
static struct region alloc_region(void const *const addr, const block_size query_size) {

    /* Вычислили вместимость для рекомендуемого размера региона */
    const block_size actual_region_size = region_actual_size(query_size);
    const block_capacity region_block_capacity = capacity_from_size(actual_region_size);

    /* Аллоцируем регион и сохраняем указатель на него */
    void *mapped_page_ptr = map_pages(addr, actual_region_size.bytes, MAP_FIXED_NOREPLACE);
    if (mapped_page_ptr == MAP_FAILED)
        mapped_page_ptr = map_pages(addr, actual_region_size.bytes, 0);
    if (mapped_page_ptr == MAP_FAILED)
        return REGION_INVALID;

    /* Разметим в блок */
    block_init(mapped_page_ptr, region_block_capacity, NULL);

    return region_init(mapped_page_ptr, actual_region_size);
}

static void *block_after(struct block_header const *block);

/**
 * Метод инициализации кучи
 * @param initial Желаемый размер кучи
 * @return
 * <p>
 * 1) Указатель на начало кучи (Если аллоцировали регион успешно)
 * <p>
 * 2) NULL (Не получилось аллоцировать регион)
 */
void *heap_init(const size_t initial) {

    /* Аллоцируем регион и возвращаем структуру */
    const struct region region = alloc_region(HEAP_START, (block_size) {initial});

    /* Проверяем структуру на корректность и возвращаем значение */
    return (region_is_invalid(&region)) ? NULL : region.addr;
}

/**
 * Минимальная вместимость блока памяти (24 байта)
 */
#define BLOCK_MIN_CAPACITY 24

/*  --- Разделение блоков --- */

/**
 * Метод индикатор разделимого блока
 * @param block Искомый блок
 * @param query_capacity Вместимость запрашиваемой блока
 * @return Статус (разделим или нет)
 */
static bool block_splittable(const struct block_header *const restrict block, const block_capacity query_capacity) {

    /* Посчитаем минимальный размер контента для разделимого блока */
    const block_size min_splittable_block_size =
            size_from_capacity((block_capacity) {query_capacity.bytes + BLOCK_MIN_CAPACITY});

    /* Возвращаем возможность разделения */
    return block->is_free && (min_splittable_block_size.bytes <= block->capacity.bytes);
}

/**
 * Метод делящий блок памяти если он достаточной большой для запрашиваемого объёма
 * <p>
 * 1) Переопределить вместимость блока
 * <p>
 * 2) Создать второй хэдер
 * <p>
 * 3) Переопределить ссылку на него
 *
 * @param block Искомый блок
 * @param query Запрашиваемый размер
 * @return Результат(разделили или нет)
 */
static bool split_if_too_big(struct block_header *restrict const block, const block_capacity query_capacity) {

    /* Проверка на разделимость */
    if (block_splittable(block, query_capacity)) {

        /* Вычислим размер второй части блока */
        const block_size second_block_size = (block_size) {block->capacity.bytes - query_capacity.bytes};

        /* Поменяли вместимость первой части блока на запрашиваемую */
        block->capacity = query_capacity;

        /* Указатель на вторую часть блока */
        void *restrict const second_block_ptr = block_after(block);

        /* Инициализируем второй хэдер */
        block_init(second_block_ptr, capacity_from_size(second_block_size), block->next);

        /* Переопределяем ссылку с первой части на вторую */
        block->next = second_block_ptr;

        return true;
    }

    return false;
}


/*  --- Слияние соседних свободных блоков --- */

/**
 * Метод для вычисления адреса начала следующего блока
 * @param block Рассматриваемый блок
 * @return Ссылку на начало следующего блока
 */
static void *block_after(struct block_header const *const block) {
    return (void *) (block->contents + block->capacity.bytes);
}

/**
 * Метод определяющий, лежат ли блоки друг за другом в памяти
 * @param fst Указатель на первый по порядку блок
 * @param snd Указатель на второй по порядку блок
 * @return Статус(идут ли друг за другом)
 */
static bool blocks_continuous(
        struct block_header const *const fst,
        struct block_header const *const snd) {
    return (void *) snd == block_after(fst);
}

/**
 * Метод определяющий, возможно ли слияние двух соседних блоков
 * @param fst Первый блок
 * @param snd Второй блок
 * @return Статус(возможно ли слияние)
 */
static bool mergeable(struct block_header const *restrict fst, struct block_header const *restrict snd) {

    /* Первый блок свободен + второй блок свободен + блоки идут друг за другом */
    return fst->is_free && snd->is_free && blocks_continuous(fst, snd);
}

/**
 * Метод объединяющий два стоящих подряд блока
 * <p>
 * 1) Перекидывает указатель искомого блока через один
 * <p>
 * 2) Увеличивает вместимость первого блока на размер второго
 *
 * @param block Блок который нужно объединить со следующим
 * @return Статус объединения
 */
static bool try_merge_with_next(struct block_header *restrict const block) {

    if (block->next) {

        /* Указатели на соседние блоки */
        struct block_header *const restrict fst = block;
        struct block_header *restrict snd = fst->next;

        if (mergeable(block, block->next)) {

            /* Перекинем указатель на следующий блок */
            fst->next = snd->next;

            /* Увеличим вместимость первого блока */
            fst->capacity.bytes += size_from_capacity(snd->capacity).bytes;

            return true;
        }
    }
    return false;
}


/*  --- Если размера кучи хватает --- */

/**
 * Результат поиска блока, который вмещает:
 * <p>
 * 1) Статус нахождения
 * <p>
 * 2) Сам блок(по возможности)
 */
struct block_search_result {
    enum {
        BSR_FOUND_GOOD_BLOCK = 0,
        BSR_REACHED_END_NOT_FOUND,
    } type;
    struct block_header *block;
};

/**
 * Метод перебора блоков памяти с целью нахождения хорошего или последнего
 * @param block Адрес начала связного списка блоков
 * @param sz Интересуемый размер
 * @return Optional тип block_header
 */
static struct block_search_result
find_good_or_last(struct block_header *restrict block, const block_capacity query_capacity) {

    /* Пройдёмся по всем хэдерам */
    while (block->next != NULL) {

        /* До конца попробуем объединить блоки */
        while (try_merge_with_next(block));

        /* Отрежем блок нужного размера если он достаточно большой */
        if (block_splittable(block, query_capacity))
            return (struct block_search_result) {.type = BSR_FOUND_GOOD_BLOCK, .block = block};

        /* Перейти по указателю дальше*/
        block = block->next;
    }

    /* Если дошли до конца, то вернём последний элемент */
    return (block_splittable(block, query_capacity))
           ? (struct block_search_result) {.type = BSR_FOUND_GOOD_BLOCK, .block = block}
           : (struct block_search_result) {.type = BSR_REACHED_END_NOT_FOUND, .block = block};
}

/**
 * Метод пытающийся выделить память в куче без расширения
 * @param block Начало кучи
 * @param query_capacity Интересуемая вместимость
 * @return Optional тип block_header
 */
static struct block_search_result
try_memalloc_existing(struct block_header *restrict block, const block_capacity query_capacity) {

    /* Найдем последний или хороший блок */
    struct block_search_result splittable_or_last_block = find_good_or_last(block, query_capacity);

    if (splittable_or_last_block.type != BSR_FOUND_GOOD_BLOCK) return splittable_or_last_block;

    /* Поделим блок */
    split_if_too_big(splittable_or_last_block.block, query_capacity);

    return splittable_or_last_block;
}

/**
 * Метод расширения кучи
 * @param last Последний элемент старой кучи
 * @param query Размер расширения
 */
static struct block_header *grow_heap(struct block_header *restrict last, const block_size query_size) {

    /* Ссылка на начало нового региона */
    void *new_region_adr = block_after(last);

    /* Аллоцируем новый регион(там сразу разметили блок) */
    const struct region new_region = alloc_region(new_region_adr, query_size);

    if (region_is_invalid(&new_region)) return NULL;

    /* Перецепим ссылку с последней ноды первого региона на первую второго региона */
    last->next = new_region.addr;

    /* Попробуем объединить последний блок первого региона с блоком второго */
    if (try_merge_with_next(last)) {
        return last;
    } else {
        return last->next;
    }
}

/**
 * Метод реализует основную логику malloc и возвращает заголовок выделенного блока
 * @param heap_start Начало кучи
 * @param query Запрашиваемый размер блока
 * @return Возвращает заголовок выделенного блока
 */
static struct block_header *
memalloc(struct block_header *restrict const heap_start, const block_capacity query_capacity) {

    /* Найдем хороший или последний элемент в искомой куче */
    struct block_search_result block_search_result = try_memalloc_existing(heap_start, query_capacity);

    /*
     * Если нашли хороший блок, то вернём его
     * Иначе, расширим кучу и возьмём свободный блок оттуда
     */
    if (block_search_result.type) {
        /* Найдем размер вместе с заголовком */
        const block_size query_size = size_from_capacity(query_capacity);

        /* Увеличим кучу на вместимость + заголовок */
        struct block_header *new_block = grow_heap(block_search_result.block, query_size);

        if (new_block == NULL) return NULL;

        /* Ещё раз поищем в куче блок памяти */
        block_search_result = try_memalloc_existing(heap_start, query_capacity);
    }

    /* Займем блок и вернём */
    block_search_result.block->is_free = false;
    return block_search_result.block;
}

void *_malloc(const size_t query) {
    const block_capacity query_capacity = (block_capacity) {query};
    struct block_header *restrict const addr = memalloc((
                                                                struct block_header *) HEAP_START, query_capacity);

    if (addr) return addr->contents;
    else return NULL;
}

/**
 * Метод для нахождения заголовка блока
 * @param contents Указатель на начало содержимого
 * @return Возвращает указатель на заголовок
 */
static struct block_header *block_get_header(const void *const contents) {
    return (struct block_header *) (((uint8_t *) contents) - offsetof(struct block_header, contents));
}

void _free(void *const mem) {

    if (!mem) return;

    struct block_header *header = block_get_header(mem);

    /* Освободим блок */
    header->is_free = true;

    /* Объединим блок со всеми следующими за ним свободными блоками */
    while (try_merge_with_next(header));
}
