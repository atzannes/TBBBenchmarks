#include "../defs.h"

/* 
 * convolution versions: This convolution does not process the 
 * rightmost and bottom-most elements of the array as we are not
 * taking care of corner cases. In fact some offsets should be 
 * re-computed because it should also leave elements at the top
 * and left of the array.
 */

#ifndef OUTER_GRAINSIZE
#define OUTER_GRAINSIZE 1
#endif

#ifndef INNER_GRAINSIZE
#define INNER_GRAINSIZE 1
#endif

#ifndef TIMES
#define TIMES 1
#endif

int image3[IMAGESIDE][IMAGESIDE];

class convBody2D {
public:
    void operator () (const tbb::blocked_range2d<int> &range) const {
        int i,j;
        for(i=range.rows().begin(); i<range.rows().end(); i++) {
            for(j=range.cols().begin(); j<range.cols().end(); j++) {
                // TODO replace this by a reducer
                int k,l;
                int dot_product = 0;
                for(k=0; k<FILTERSIDE; k++){
                    for(l=0; l<FILTERSIDE; l++){
                        dot_product = dot_product + image[i+k][j+l]*filter[k][l];
                    }
                }
                image3[i+FILTERSIDE/2][j+FILTERSIDE/2] = image[i+FILTERSIDE/2][j+FILTERSIDE/2] + dot_product;
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

    int bound = IMAGESIDE - FILTERSIDE;
    PARTITIONER partitioner;
    tbb::tick_count t0 = tbb::tick_count::now();

    for (int i=0; i<TIMES; i++) {
        tbb::parallel_for (tbb::blocked_range2d<int>(0,bound,OUTER_GRAINSIZE,0,bound,INNER_GRAINSIZE), convBody2D(), partitioner);
    }
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

    // Dump result
    hexdump_var("result.hex", (int*)image3, IMAGESIDE*IMAGESIDE);

#ifdef PRINT_RESULT
  int x,y;
  for(x=0; x<IMAGESIDE; x++) {
    
    for(y=0; y<IMAGESIDE; y++) {
      printf("%d,", image3[x][y]);
    }
    printf("\n");
  }
#endif 
  return 0;
}
