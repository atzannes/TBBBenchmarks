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
                for(k=0;k<SIZE_Z;k++){
                    sum += (X[i][k]-5)*(Y[k][j]-5);
                }
                Z[i][j] = sum;
            }
        }
    }
};

void
print_array(int A[SIZE_X][SIZE_X]) {
    for (int i=0; i<SIZE_X; i++) {
        for (int j=0; j<SIZE_X; j++) {
            printf("%d,", A[i][j]);
        }
        printf("\n");
    }
}

int Z[SIZE_X][SIZE_Z] = {0};

int X[SIZE_X][SIZE_Y] = {0};

int Y[SIZE_Y][SIZE_Z] = {0};
#include "../xboutils/xboutils.h"

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

    // initialize input arrays X and Y from .xbo file
    char* fname, *vname;
    int length, offset, header_length;

    fname = "4/mat.xbo";
    vname = "X";
    length = get_var_length_xbo(fname, vname);
    offset = get_var_offset_xbo(fname, vname);
    header_length = get_xbo_header_length (fname);
    fprintf (stdout, "DEBUG:: .xbo file = %s\n", fname);
    fprintf (stdout, "DEBUG:: %s length = %d\n", vname, length);
    fprintf (stdout, "DEBUG:: %s offset = %d\n", vname, offset);
    fprintf (stdout, "DEBUG:: header length = %d\n", header_length);

    read_int_data_var(fname, X, "X");
    read_int_data_var(fname, Y, "Y");
    printf ("Printing X:\n");
    print_array(X);

    printf ("Printing Y:\n");
    print_array(Y);


    int j,k;
    PARTITIONER partitioner;
    tbb::tick_count t0 = tbb::tick_count::now();

    tbb::parallel_for (tbb::blocked_range2d<int,int>(0,SIZE_X,OUTER_GRAINSIZE,0,SIZE_Y,INNER_GRAINSIZE), matmultBody2D(), partitioner);

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

