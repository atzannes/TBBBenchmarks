#include "tbb/partitioner.h"
#include "tbb/blocked_range.h"
#include "tbb/blocked_range2d.h"
#include "tbb/tick_count.h"
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"

#include <stdio.h>
#include <stdlib.h>

/************************************************************************************
 * Floyd Warshall All-Pairs Shortest Path
 *  #define SERIAL for serial execution
 *  #define PARALLEL_1D for 2 levels of nested parallel_for
 *  default is PARALLEL_2D with 1 level parallel_for with 2D Range
 * 
 *  SIZE determines the size of the input
 *  GRAINSIZE determines the sst and ppt
 *  PARTITIONER defaults to tbb::simple_partitioner()
 *************************************************************************************/
#ifndef INIT_OUTER_GRAINSIZE
#define INIT_OUTER_GRAINSIZE 1
#endif

#ifndef INIT_INNER_GRAINSIZE
#define INIT_INNER_GRAINSIZE 1
#endif

#ifndef COMPUTE_OUTER_GRAINSIZE
#define COMPUTE_OUTER_GRAINSIZE 1
#endif

#ifndef COMPUTE_INNER_GRAINSIZE
#define COMPUTE_INNER_GRAINSIZE 1
#endif

#ifndef SIZE
#define SIZE 10
#endif

#ifndef TIMES
#define TIMES 1
#endif


#include "../defs.h"

#ifdef INIT  // true if we are initializing the data instead of loading it
// adjacency matrix with weights
// w_ij = weight of edge (i,j) if it exists, 0 if i==j, and +inf otherwise
int W[SIZE][SIZE]; // defined in allpairs.h

// result array that contains shortest path weight
// r_ij = weight of shortest path from i to j
int R[SIZE][SIZE]; // defined in allpairs.h

// result array that contains predecessor information
// p_ij = predecessor of j on the shortest path from i to j
int P[SIZE][SIZE]; // defined in allpairs.h

void initWeights(int W[SIZE][SIZE])
{
    int i;
    for(i=0; i<SIZE; i++) {
        int j=0;
        for(j=0; j<SIZE; j++) {
            W[i][j] = (i-j)*(i-j);
        }
    }
}
#endif /* INIT */



#define min(x,y) ((x<y)?x:y)
#define NIL  -1
#define MAX_INT 0x7fffffff

// CLR90 ex 26.2.2 p563-564
void serial_floydWarshall(int W[SIZE][SIZE], int R[SIZE][SIZE], int P[SIZE][SIZE])
{
    int i,j;
    // initialize R and P
    for(i=0; i<SIZE; i++) {
        for(j=0; j<SIZE; j++) {
            R[i][j] = W[i][j];
            if (i==j || W[i][j] < MAX_INT)
                P[i][j] = i;
            else
                P[i][j] = NIL;
        }
    }
    // compute R
    int k;
    for(k=0; k<SIZE; k++) {
        for(i=0; i<SIZE; i++) {
            //if (i!=k) { // we don't check 'i' here to allow flattening
                for(j=0; j<SIZE; j++) {
                    if (j!=k && i!=k) { // we check 'i' here to allow flattening
                        int sum = R[i][k]+R[k][j];
                        if (R[i][j] > sum) {
                            R[i][j] = sum;
                            P[i][j] = P[k][j];
                        }
                    }
                }
            //} // end if (i!=k)
        }
    }
}


struct InitializeResultAndPredecessor2DBody {
    void operator()( const tbb::blocked_range2d<int> &range ) const {
        int i,j;
        for(i=range.rows().begin(); i<range.rows().end(); i++) {
            for(j=range.cols().begin(); j<range.cols().end(); j++) {
                R[i][j] = W[i][j];
                if (i==j || W[i][j] < MAX_INT)
                    P[i][j] = i;
                else
                    P[i][j] = NIL;
            }
        }
    }
};

class InitializeResultAndPredecessor1DNestedBody {
public:
    int i;
    void operator () ( const tbb::blocked_range<int> &range ) const {
            int j;
            for(j=range.begin(); j<range.end(); j++) {
                R[i][j] = W[i][j];
                if (i==j || W[i][j] < MAX_INT)
                    P[i][j] = i;
                else
                    P[i][j] = NIL;
            }
    }
    // Constructor
    InitializeResultAndPredecessor1DNestedBody(int i) {
        this->i = i;
    }
};

struct InitializeResultAndPredecessor1DBody {
    void operator()( const tbb::blocked_range<int> &range ) const {
        int i,j;
        PARTITIONER partitioner;
        for(i=range.begin(); i<range.end(); i++) {
            tbb::parallel_for(tbb::blocked_range<int> (0,SIZE,INIT_INNER_GRAINSIZE), InitializeResultAndPredecessor1DNestedBody(i), partitioner);
        }
    }
};

class ComputeResultAndPredecessor2DBody {
public:
    int k;
    void operator()( const tbb::blocked_range2d<int> &range ) const {
        int i,j;
        for(i=range.rows().begin(); i<range.rows().end(); i++) {
            //if (i!=k) { // we don't check 'i' here to allow flattening
                for(j=range.cols().begin(); j<range.cols().end(); j++) {
                    if (j!=k && i!=k) { // we check 'i' here to allow flattening
                        int sum = R[i][k]+R[k][j];
                        if (R[i][j] > sum) {
                            R[i][j] = sum;
                            P[i][j] = P[k][j];
                        }
                    }
                }
            //} // end if (i!=k)
        }
    }
 
    // constructor
    ComputeResultAndPredecessor2DBody(int k) {
        this->k = k;
    }
};

class ComputeResultAndPredecessor1DNestedBody {
public:
    int k,i;
    void operator()( const tbb::blocked_range<int> &range ) const {
        int j;
        for(j=range.begin(); j<range.end(); j++) {
            if (j!=k && i!=k) { // we check 'i' here to allow flattening
                int sum = R[i][k]+R[k][j];
                if (R[i][j] > sum) {
                    R[i][j] = sum;
                    P[i][j] = P[k][j];
                }
            }
        }
    }
 
    // constructor
    ComputeResultAndPredecessor1DNestedBody(int k, int i) {
        this->k = k;
        this->i = i;
    }
};

class ComputeResultAndPredecessor1DBody {
public:
    int k;
    void operator()( const tbb::blocked_range<int> &range ) const {
        int i;
        PARTITIONER partitioner;
        for(i=range.begin(); i<range.end(); i++) {
            tbb::parallel_for( tbb::blocked_range<int> (0,SIZE,COMPUTE_INNER_GRAINSIZE), ComputeResultAndPredecessor1DNestedBody(k,i), partitioner );
        }
    }
 
    // constructor
    ComputeResultAndPredecessor1DBody(int k) {
        this->k = k;
    }
};

void floydWarshall2d(int W[SIZE][SIZE], int R[SIZE][SIZE], int P[SIZE][SIZE])
{
    tbb::blocked_range2d<int> init_r(0,SIZE,INIT_OUTER_GRAINSIZE,0,SIZE,INIT_INNER_GRAINSIZE);
    PARTITIONER partitioner;
    tbb::parallel_for (init_r, InitializeResultAndPredecessor2DBody(), partitioner );
    // compute R and P
    tbb::blocked_range2d<int> compute_r(0,SIZE,COMPUTE_OUTER_GRAINSIZE,0,SIZE,COMPUTE_INNER_GRAINSIZE);
    int k;
    for(k=0; k<SIZE; k++) {
        tbb::parallel_for (compute_r, ComputeResultAndPredecessor2DBody(k), partitioner );
    }
}

void floydWarshall1d(int W[SIZE][SIZE], int R[SIZE][SIZE], int P[SIZE][SIZE])
{
    tbb::blocked_range<int> init_r(0,SIZE,INIT_OUTER_GRAINSIZE);
    PARTITIONER partitioner;
    tbb::parallel_for (init_r, InitializeResultAndPredecessor1DBody(), partitioner );
    // compute R and P
    tbb::blocked_range<int> compute_r(0,SIZE,COMPUTE_OUTER_GRAINSIZE);
    int k;
    for(k=0; k<SIZE; k++) {
        tbb::parallel_for (compute_r, ComputeResultAndPredecessor1DBody(k), partitioner );
    }
}

void printPath(int i, int j, int P[SIZE][SIZE]) {
    int pred = P[i][j];
    if (pred==NIL) {
        printf("NO PATH");
    } else if (pred==i) {
        printf("%d, %d", i, j);
    } else {
        printPath(i, pred, P);
        printf(", %d", j);
    }
}

int main(int argc, char* argv[]) 
{
    int nth=-1; // number of (hardware) threads (-1 means undefined)
    if (argc > 1) { //command line with parameters
        if (argc > 2) {
            printf("ERROR: wrong use of command line arguments. Usage %s <#threads>\n", argv[0]);
            return 1;
        } else {
            nth = atoi( argv[1] );
        }        
    }

    int i,j;
#ifdef INIT
    initWeights(W);
#endif /* INIT */

#ifdef DEBUG
    printf("W :: \n");
    for(i=0; i<SIZE; i++){
        for(j=0; j<SIZE; j++) {
            printf("%d ", W[i][j]);
        }
        printf("\n");
    }
    printf("\n");
#endif

#ifdef SERIAL
    tbb::tick_count t0 = tbb::tick_count::now();
    for (i=0; i<TIMES; i++) 
        serial_floydWarshall (W, R, P);
#else // PARALLEL
    int defth = tbb::task_scheduler_init::default_num_threads();
    if (nth<0) nth=defth;
    printf("Default #Threads=%d. Using %d threads\n", defth, nth);
    tbb::task_scheduler_init init(nth);
    tbb::tick_count t0 = tbb::tick_count::now();
    for (int i=0; i<TIMES; i++)
  #ifdef PARALLEL_1D // PARALLEL nested
        floydWarshall1d(W, R, P);
  #else  // PARALLEL w. 2D range
        floydWarshall2d(W, R, P);
  #endif
#endif
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

    // Dump result
    hexdump_var("result.hex", (int*)R, SIZE*SIZE);

#ifdef PRINT_RESULT
    printf("R :: \n");
    for(i=0; i<SIZE; i++){
        for(j=0; j<SIZE; j++) {
            printf("%d ", R[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    printf("P :: \n");
    for(i=0; i<SIZE; i++){
        for(j=0; j<SIZE; j++) {
            printf("%d ", P[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    for(i=0; i<SIZE; i++){
        for(j=0; j<SIZE; j++) {
            printf("Path[%d,%d]=", i,j);
            printPath(i,j,P);
            printf("\n");
        }
    }
#endif

    return 0;
}
