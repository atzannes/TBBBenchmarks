#include "../defs.h"

#ifndef TIMES
#define TIMES 1
#endif

int    rowptr[SIZE_M+1];
int    col_ind[NUMBER_NON_ZERO];
int    values[NUMBER_NON_ZERO];
int    vector[SIZE_N];
int    result[SIZE_M];

int main(void){

    int i,j;
    read_int_data_var (XBOFILE, rowptr,  "rowptr");
    //hexdump_var("rowptr.hex", rowptr, SIZE_M+1);

    read_int_data_var (XBOFILE, col_ind, "col_ind");
    //hexdump_var ("col_ind.hex", col_ind, NUMBER_NON_ZERO);

    read_int_data_var (XBOFILE, values,  "values");
    //hexdump_var ("values.hex", values, NUMBER_NON_ZERO);

    read_int_data_var (XBOFILE, vector,  "vector");
    //hexdump_var ("vector.hex", vector, SIZE_N);

    //printf("DEBUG:: read XBO input, now on to compute stuff...\n");

    tbb::tick_count t0 = tbb::tick_count::now();
    for (int c=0; c<TIMES; c++) {
        for (i=0; i<SIZE_M; i ++) {
            result[i] = 0;
            for(j=rowptr[i]; j<rowptr[i+1]; j++) {
                result[i] += values[j] * vector[col_ind[j]];
            }
        }
    }
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

    // Dump result
    hexdump_var("result.hex", result, SIZE_M);

#ifdef PRINT_RESUL
    int i;
    for(i=0;i<SIZE_N;i++)
        printf("%d\n",result[i]);
#endif
    return 0;
}
