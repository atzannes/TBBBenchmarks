#include "tbb/tick_count.h"
#include <stdio.h>

//#define PRINT_RESULT 1

// CLR 1st edition p. 154
// rearrange subarray A[p..r]
int do_partition(int* A, int p, int r) {
        int x = A[p];
        int i = p - 1;
        int j = r+1;
	while(1) {
	  do { j--; } while (A[j]>x); 
	  do { i++; } while (A[i]<x);
	  if (i<j) { // exchange A[i] <-> A[j]
	    int tmp = A[j];
	    A[j] = A[i];
	    A[i] = tmp;
	  } else return j;
	}
}

//CLR 1st edition p. 154
// Sort the array A[p..r]
void quicksort(int *A, int p, int r) {
	if (p < r) {
		int q = do_partition(A, p, r);
		quicksort(A, p, q);
		quicksort(A, q + 1, r);
	}
}

void print_array(int *A, int size) {
  int i;
  for (i=0;i<size;i++) 
    printf("%d\n",A[i]);
}

#ifndef N
  #define N A_dim0_size
#endif 

int main(void) {
    tbb::tick_count t0 = tbb::tick_count::now();
    quicksort(A,0,N-1);
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());


#ifdef PRINT_RESULT
  print_array(A, n); 
#endif
 return 0;
}
