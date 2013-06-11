/* Serial Matrix multiplication. 
 * Integer version. 
 */
//#include <workstealHW-sched.c>
#include "../defs.h"

int main(void){
    int i,j,k;
    
    tbb::tick_count t0 = tbb::tick_count::now();
    for(i=0;i<SIZE_X;i++) {
        for(j=0; j<SIZE_Y; j++) {
            int sum=0;
            for(k=0;k<SIZE_Z;k++){
                sum += (X[i][k]-5)*(Y[k][j]-5);
            }
            Z[i][j] = sum;
        }
    }
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

