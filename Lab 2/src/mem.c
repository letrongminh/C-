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
void debug(const char* fmt, ... );

extern inline block_size size_from_capacity( block_capacity cap );
extern inline block_capacity capacity_from_size( block_size sz );

static bool            block_is_big_enough( size_t query, struct block_header* block ) { return block->capacity.bytes >= query; }
static size_t          pages_count   ( size_t mem )                      { return mem / getpagesize() + ((mem % getpagesize()) > 0); }
static size_t          round_pages   ( size_t mem )                      { return getpagesize() * pages_count( mem ) ; }


static void block_init( void* restrict addr, block_size block_sz, void* restrict next ) {
  *((struct block_header*)addr) = (struct block_header) {
    .next = next,
    .capacity = capacity_from_size(block_sz),
    .is_free = true
  };
}

static size_t region_actual_size( size_t query ) {
    return size_max( round_pages( query ), REGION_MIN_SIZE );
}

extern inline bool region_is_invalid( const struct region* r );

 void* map_pages_for_main(void const* addr, size_t length, int additional_flags) {
    return mmap( (void*) addr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | additional_flags , 0, 0 );
}

static void* map_pages(void const* addr, size_t length, int additional_flags) {
  return mmap( (void*) addr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | additional_flags , 0, 0 );
}


/*  аллоцировать регион памяти и инициализировать его блоком */
static struct region alloc_region  ( void const * addr, size_t query ) {
    /*  ??? */
    struct region  region_sz;

    region_sz.size=region_actual_size(query);

    region_sz.addr=map_pages(addr,region_actual_size(query), MAP_FIXED_NOREPLACE );

    if(region_sz.addr==(void*)-1){//MAP_FAILED
        region_sz.addr=map_pages(addr,region_actual_size(query), 0 );

        region_sz.extends=false;

    }
    else{
        region_sz.extends=true;
    }

    block_size size_for_current_function={.bytes=region_actual_size(query)};
    block_init(region_sz.addr,size_for_current_function,NULL);
        return region_sz;
    }

static void* block_after( struct block_header const* block )         ;

void* heap_init( size_t initial ) {
    const struct region region = alloc_region( HEAP_START, initial );
    if ( region_is_invalid(&region) ) return NULL;

    return region.addr;
}

#define BLOCK_MIN_CAPACITY 24

/*  --- Разделение блоков (если найденный свободный блок слишком большой )--- */

static bool block_splittable( struct block_header* restrict block, size_t query) {
    return block-> is_free && query + offsetof( struct block_header, contents ) + BLOCK_MIN_CAPACITY <= block->capacity.bytes;
}

static bool split_if_too_big( struct block_header* block, size_t query ) {
    /*  ??? */

    if(block_splittable(block,query)){
        block_init(
                block->contents+query,(block_size){block->capacity.bytes-query},
                block->next);

        block->capacity=(block_capacity){query};
        block->next=(struct block_header*)(block->contents+query);

        return true;
    }
    else
        return false;
}


/*  --- Слияние соседних свободных блоков --- */

static void* block_after( struct block_header const* block )              {
    return  (void*) (block->contents + block->capacity.bytes);
}
static bool blocks_continuous (struct block_header const* fst,
                                struct block_header const* snd ){
        return (void*)snd == block_after(fst);
}

static bool mergeable(struct block_header const* restrict fst, struct block_header const* restrict snd) {
        return fst->is_free && snd->is_free && blocks_continuous( fst, snd ) ;
}

static bool try_merge_with_next( struct block_header* block ) {

    /*  ??? */

  if(block->next&&mergeable(block,block->next) ){
      struct block_header *header_next=block->next;
      block->next=header_next->next;
      block_size size_for_current_function={.bytes=size_from_capacity(header_next->capacity).bytes};
      block->capacity.bytes+=size_for_current_function.bytes;
      return true;
  } else
      return false;
}


/*  --- ... ecли размера кучи хватает --- */

struct block_search_result {
  enum {BSR_FOUND_GOOD_BLOCK, BSR_REACHED_END_NOT_FOUND, BSR_CORRUPTED} type;
  struct block_header* block;
};


static struct block_search_result find_good_or_last  ( struct block_header* restrict block, size_t sz )    {
  /*??? */
    if(block==NULL)
        return (struct block_search_result){BSR_CORRUPTED,NULL};
    while (block!=NULL){
        for(;try_merge_with_next(block);){
        }
        if(block->is_free&&block_is_big_enough(sz,block)){
            return (struct block_search_result) {BSR_FOUND_GOOD_BLOCK, block};
        } else
        {
            if(block->next==NULL)
                break;
            block=block->next;
        }

    }
    return (struct block_search_result){.type=BSR_REACHED_END_NOT_FOUND,.block=block};
}

/*  Попробовать выделить память в куче начиная с блока `block` не пытаясь расширить кучу
 Можно переиспользовать как только кучу расширили. */
static struct block_search_result try_memalloc_existing ( size_t query, struct block_header* block ) {
  struct block_search_result existing_block=find_good_or_last(block,query);
  if(existing_block.type==BSR_FOUND_GOOD_BLOCK) {
      split_if_too_big(existing_block.block, query);
      existing_block.block->is_free=false;
  }
    return existing_block;
}



static struct block_header* grow_heap( struct block_header* restrict last, size_t query ) {
  /*  ??? */
  struct region new_region=alloc_region(block_after(last),query);
  last->next=new_region.addr;
    return (struct block_header*)new_region.addr;
}

/*  Реализует основную логику malloc и возвращает заголовок выделенного блока */
static struct block_header* memalloc( size_t query, struct block_header* heap_start) {
  /*  ??? */
    if(query<BLOCK_MIN_CAPACITY)
        query=BLOCK_MIN_CAPACITY;
    //const size_t size = region_actual_size(query);
    struct block_search_result result=try_memalloc_existing(query,heap_start);
   if(result.type==BSR_FOUND_GOOD_BLOCK) {
       result.block->is_free = false;
       return result.block;
   }
   if(result.type==BSR_REACHED_END_NOT_FOUND){

       result = try_memalloc_existing(query,grow_heap(result.block,query));
       if(result.type==BSR_FOUND_GOOD_BLOCK){
           result.block->is_free=false;
           return result.block;
           }
   }
   if(result.type==BSR_CORRUPTED)
       return NULL;
    return NULL;
}

void* _malloc( size_t query ) {
  struct block_header* const addr = memalloc( query, (struct block_header*) HEAP_START );
  if (addr) return addr->contents;
  else return NULL;
}

static struct block_header* block_get_header(void* contents) {
  return (struct block_header*) (((uint8_t*)contents)-offsetof(struct block_header, contents));
}

void _free( void* mem ) {
  if (!mem) return ;
  struct block_header* header = block_get_header( mem );
  header->is_free = true;
  /*  ??? */
  while (header->next&&try_merge_with_next(header)){};
}
