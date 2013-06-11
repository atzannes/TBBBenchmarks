/**
 *  Compute the product of an NxM sparse matrix by a dense Mx1 vector
 */
#include "../defs.h"

#ifndef TIMES
#define TIMES 1
#endif

#ifndef OUTER_GRAINSIZE
#define OUTER_GRAINSIZE 1
#endif

#ifndef INNER_GRAINSIZE
#define INNER_GRAINSIZE 1
#endif

int    rowptr[SIZE_M+1];
int    col_ind[NUMBER_NON_ZERO];
int    values[NUMBER_NON_ZERO];
int    vector[SIZE_N];
int    result[SIZE_M];
tbb::atomic<int> resultAtomic[SIZE_M];


// The reduce operation updates local state that is then combined 
// with parent thread
class reduceBody {
public:
    int value;

    // Constructor
    reduceBody(): value(0) {}

    // Split Constructor
    reduceBody(reduceBody &b, tbb::split) { value = 0;}

    // Reduce operation
    void operator () (const tbb::blocked_range<int> &range) {
        int i;
        int tmp = 0;
        for(i=range.begin(); i<range.end(); i++) {
            tmp += values[i] * vector[col_ind[i]];
        }
        value += tmp;
    }

    // Join Operation
    void join (reduceBody &rhs) { value += rhs.value;}
    
};

// TODO: Reduce with a reference to the memory location to update
// This should be faster than the "reduceBody" implementation
class reduceBodyRef{
public:
    int idx;
    // tbb::atomic<int> value;

    // Constructor
    reduceBodyRef(int x): idx(x) { }
    //reduceBodyRef(){ value = 0; }

    // Split Constructor
    reduceBodyRef(reduceBodyRef &b, tbb::split) { idx = b.idx; }
    //reduceBodyRef(reduceBodyRef &b, tbb::split) { value = 0;}

    // Reduce operation
    void operator () (const tbb::blocked_range<int> &range) {
        int i;
        int tmp = 0;
        for(i=range.begin(); i<range.end(); i++) {
            tmp += values[i] * vector[col_ind[i]];
        }
        resultAtomic[idx] += tmp; // atomic
        //value += tmp; // atomic
    }

    // Join Operation
    void join (reduceBodyRef& rhs) { }
    //void join (reduceBodyRef& rhs) { value += rhs.value; }
    
};


class spmvOuterBody {
public:
    void operator () (const tbb::blocked_range<int> &range) const {
        int i;
        PARTITIONER partitioner;
        for(i=range.begin(); i<range.end(); i++) {
            

#ifdef SERIAL_REDUCTION
            // Serial version
            result[i] = 0;
            for(int j=rowptr[i]; j<rowptr[i+1]; j++) {
                result[i] += values[j] * vector[col_ind[j]];
            }
#else
#ifdef TBB_REDUCTION
            // ReduceBody
            result[i] = 0;
            reduceBody reduce;
            tbb::parallel_reduce (tbb::blocked_range<int> (rowptr[i],rowptr[i+1], INNER_GRAINSIZE), reduce, partitioner);
            result[i] = reduce.value;
#else // XMT/PSM_REDUCTION
            resultAtomic[i] = 0;
            // ReduceBodyRef
            reduceBodyRef reduceRef(i);
            //reduceBodyRef reduceRef;
            tbb::parallel_reduce (tbb::blocked_range<int> (rowptr[i],rowptr[i+1], INNER_GRAINSIZE), reduceRef, partitioner);
            //result[i] = reduceRef.value;
#endif
#endif


        }
    }
    
    // Constructor
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

    PARTITIONER partitioner;
    read_int_data_var (XBOFILE, rowptr,  "rowptr");
    read_int_data_var (XBOFILE, col_ind, "col_ind");
    read_int_data_var (XBOFILE, values,  "values");
    read_int_data_var (XBOFILE, vector,  "vector");
    //read_int_data_var (XBOFILE, result,  "result");

    tbb::tick_count t0 = tbb::tick_count::now();
    for (int c=0; c<TIMES; c++)
        tbb::parallel_for (tbb::blocked_range<int> (0, SIZE_M, OUTER_GRAINSIZE), spmvOuterBody(), partitioner);
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

#if !defined SERIAL_REDUCTION && !defined TBB_REDUCTION
    // copy resultAtomic to result
    for (int c=0; c<SIZE_M; c++) result[c] = resultAtomic[c];
#endif

    // Dump result
    hexdump_var("result.hex", (int*)result, SIZE_M);

#ifdef PRINT_RESULT
    int i;
    for(i=0;i<m;i++)
        printf("%d\n",result[i]);
#endif
    return 0;
}
