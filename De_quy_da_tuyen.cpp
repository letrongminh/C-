#include<iostream>
using namespace std;
void print_arr(int arr[], int n){
	for(int i=0; i < n; i++)
		{
		cout<<arr[i]<<"\t";
		}
	cout<<endl;
}
void print_per(int arr[], int n, int i)
{
	int j, swap;
	print_arr(arr, n);
	for (j=i+1; j<n; j++)
		if(arr[j]<arr[i])
		{
			swap = arr[i];
			arr[i]=arr[j];
			arr[j]= swap;
		}
	print_per(arr, n, i+1);
}
int main()
{
	int M[3];
	M[0]=2;
	M[1]=6;
	M[2]=0;
	print_per(M, 3, 0);
	return 0;
}
