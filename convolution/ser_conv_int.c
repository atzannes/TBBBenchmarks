#include "../defs.h"

/* 
 * convolution versions: This convolution does not process the 
 * rightmost and bottom-most elements of the array as we are not
 * taking care of corner cases. In fact some offsets should be 
 * re-computed because it should also leave elements at the top
 * and left of the array.
 */

#ifndef TIMES
#define TIMES 1
#endif

int image3[IMAGESIDE][IMAGESIDE];

int main(void) {
    int bound;
    int value;

    tbb::tick_count t0 = tbb::tick_count::now();
    bound = IMAGESIDE - FILTERSIDE - 1;

  for(int c=0; c<TIMES; c++) {
    int i,j;
    for (i=0; i<= bound; i++) {
        for(j=0; j<=bound; j++) {
            int k,l;
            int dot_product;
    
            dot_product = 0;

            for(k=0; k<FILTERSIDE; k++){
                for(l=0; l<FILTERSIDE; l++){
                    dot_product = dot_product + image[i+k][j+l]*filter[k][l];
                }
            }
            image3[i+FILTERSIDE/2][j+FILTERSIDE/2] = image[i+FILTERSIDE/2][j+FILTERSIDE/2] + dot_product;
        }
    }
  }
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

    // Dump result
    hexdump_var("result.hex", (int*)image3, IMAGESIDE*IMAGESIDE);

    /* printf("Done.\n"); */
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
