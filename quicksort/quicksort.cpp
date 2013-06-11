#include "../defs.h"

#ifndef SIZE
  #define SIZE A_dim0_size
#endif

// if the subarray to sort is smaller than QSORT_THRESHOLD 
// ser_quicksort is called on it. This implements the 
// parallelism cutoff for quicksort
#ifndef QSORT_THRESHOLD
#define QSORT_THRESHOLD 100
#endif

// if the array to partition is smaller than PARTITION_THRESHOLD
// ser_partition (instead of parallel_partition) is called on it.
// This implements the parallelism cutoff for partition.
// Note that this is only used when PARALLELIZE_PARTITION is defined
#ifndef PARTITION_THRESHOLD
//#define PARTITION_THRESHOLD 16384
//#define PARTITION_THRESHOLD 50000
#define NPROC 64
#define PARTITION_THRESHOLD (3*N/NPROC)
#endif 

// PARTITION_CLUST is the grainsize for the 2 spawns in partition
// if it is not defined the compiler may try to do automatic clustering
// to determine the grainsize.
//#ifndef PARTITION_CLUST
//#define PARTITION_CLUST 500
//#endif

//#define PRINT_RESULT 1


int B[SIZE];

/** 
 *  Code for parallelizing partition

psBaseReg ltcounter;
psBaseReg gtcounter;

// rearrange subarray A[p..r]

int do_partition(int *A, int p, int r) {
    int pivot = A[p];
    int ltcounter, gtcounter;
    ltcounter = p;
    gtcounter = r;
#ifdef PARTITION_CLUST
    spawn(p+1,r,PARTITION_CLUST) {
#else
    spawn(p+1,r) {
#endif
         int tmp;
         if (A[$]>pivot) {
             tmp = -1;
             psm(tmp,gtcounter);
             B[tmp] = A[$];
         } else if (A[$]<pivot) {
             tmp = 1;
             psm(tmp,ltcounter);
             B[tmp] = A[$];
         }
    }
    // copy back
#ifdef PARTITION_CLUST
    spawn(p,r,PARTITION_CLUST) { 
#else
    spawn(p,r) {
#endif
        if ($<ltcounter || $>gtcounter)
            A[$] = B[$];
        else
            A[$] = pivot;
    }
    return gtcounter; 
}
*/

// CLR 1st edition p. 154
// rearrange subarray A[p..r]
int ser_do_partition(int* A, int p, int r) {
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

void quicksort(int *A, int p, int r);
void ser_quicksort(int *A, int p, int r);

//CLR 1st edition p. 154
// Sort the array A[p..r]
class QuicksortFunctor {
public:
    int* A;
    int p;
    int r;

    // Constructor
    QuicksortFunctor(int* A, int p, int r): A(A), p(p), r(r) {};

    void operator() () const { 
        if ( r-p < QSORT_THRESHOLD ) 
            ser_quicksort(A,p,r);
        else
            quicksort(A,p,r); 
    }
};

void quicksort(int *A, int p, int r) {
    if (p < r) {
        int q;
#ifdef PARALLELIZE_PARTITION
        if (r-p < PARTITION_THRESHOLD) q = ser_do_partition(A,p,r);
        else q = do_partition(A, p, r);
#else
        q = ser_do_partition(A,p,r);
#endif
        
        QuicksortFunctor left (A,p,q);
        QuicksortFunctor right(A,q+1,r);
        tbb::parallel_invoke(left,right);
    }
}

void ser_quicksort(int *A, int p, int r) {
    if (p < r) {
        int q = ser_do_partition(A, p, r);
	ser_quicksort(A, p, q);
	ser_quicksort(A, q + 1, r);
    }
}

void print_array(int *A, int size) {
  int i;
  for (i=0;i<size;i++) 
    printf("%d\n",A[i]);
}

int main(int argc, char* argv[])
{
#ifdef SERIAL
    tbb::tick_count t0 = tbb::tick_count::now();
    ser_quicksort(A,0,SIZE-1);
    tbb::tick_count t1 = tbb::tick_count::now();
#else
    int nth=-1; // number of (hardware) threads (-1 means undefined)
    if (argc > 1) { //command line with parameters
        if (argc > 2) {
            printf("ERROR: wrong use of command line arguments. Usage %s <#threads>\n", argv[0]);
            return 1;
        } else {
            nth = atoi( argv[1] );
        }
    }
    int defth = tbb::task_scheduler_init::default_num_threads();
    if (nth<0) nth=defth;
    printf("Default #Threads=%d. Using %d threads\n", defth, nth);
    tbb::task_scheduler_init init(nth);
    tbb::tick_count t0 = tbb::tick_count::now();
    quicksort(A,0,SIZE-1);
    tbb::tick_count t1 = tbb::tick_count::now();
#endif
    printf("Ticks = %f\n", (t1-t0).seconds());
 
#ifdef PRINT_RESULT
    print_array(A, n); 
#endif

}
