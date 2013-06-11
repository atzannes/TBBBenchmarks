/* Matrix multiplication. 
 */
#include "../defs.h"

#ifndef OUTER_GRAINSIZE
#define OUTER_GRAINSIZE 1
#endif

#ifndef INNER_GRAINSIZE
#define INNER_GRAINSIZE 1
#endif

class matmultBody2D {
public:
    void operator () (const tbb::blocked_range2d<int> &range) const {
        int i,j;
        for(i=range.rows().begin(); i<range.rows().end(); i++) {
            for(j=range.cols().begin(); j<range.cols().end(); j++) {
                int k;
                int sum=0;
                for(k=0;k<SIZE_Y;k++){
                    sum += (X[i][k]-5)*(Y[k][j]-5);
                }
                Z[i][j] = sum;
            }
        }
    }
};

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
    int defth = tbb::task_scheduler_init::default_num_threads();
    if (nth<0) nth=defth;
    printf("Default #Threads=%d. Using %d threads\n", defth, nth);
    tbb::task_scheduler_init init(nth);


    int j,k;
    PARTITIONER partitioner;
    tbb::tick_count t0 = tbb::tick_count::now();

    tbb::parallel_for (tbb::blocked_range2d<int,int>(0,SIZE_X,OUTER_GRAINSIZE,0,SIZE_Z,INNER_GRAINSIZE), matmultBody2D(), partitioner);

    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

    // Dump result
    hexdump_var("result.hex", (int*)Z, SIZE_X*SIZE_Z);

#ifdef PRINT_RESULT
    //printf("Done!\n");
     
     for(j=0;j<N;j++) { 
         for(k=0;k<N;k++) { 
             printf("%d,",Z[j][k]); 
        }
        printf("\n");
    }
#endif
    return 0;
}

