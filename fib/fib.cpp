/*******************************************/
/*    Fibonacci                            */
/*******************************************/

#include "../defs.h"

// N 
#ifndef NUMBER
#define NUMBER 400
#endif

long SerialFib( long n ) {
    if( n<=2 )
        return 1;
    else
        return SerialFib(n-1)+SerialFib(n-2);
}

struct FibContinuation: public tbb::task {
    long* const sum;
    long x, y;
    FibContinuation( long* sum_ ) : sum(sum_) {}
    task* execute() {
        *sum = x+y;
        return NULL;
    }
};

struct FibTask: public tbb::task {
    const long n;
    long* const sum;
    FibTask( long n_, long* sum_ ) : n(n_), sum(sum_) {}

    task* execute() {
        if( n<=2 ) {
            //*sum = SerialFib(n);
            *sum = 1;
            return NULL;
        } else {
            FibContinuation& c =
                *new( allocate_continuation() ) FibContinuation(sum);
            FibTask& a = *new( c.allocate_child() ) FibTask(n-2,&c.x);
            FibTask& b = *new( c.allocate_child() ) FibTask(n-1,&c.y);
            // Set ref_count to "two children".
            c.set_ref_count(2);
            c.spawn( b );
            return &a;
        }
    }
};


long ParallelFib( long n ) {
    long sum;
    FibTask& a = *new(tbb::task::allocate_root()) FibTask(n,&sum);
    tbb::task::spawn_root_and_wait(a);
    return sum;
}

class reduceBody {
public:
    long x;
    long value;
    // Constructor
    reduceBody(long x_) : x(x_), value(0) {} 

    // Split Constructor
    reduceBody(reduceBody &b, tbb::split) : x(b.x), value(0) {} // TODO

    // Reduce Operation
    void operator () (const tbb::blocked_range<int> &range) {
        for (int i = range.begin(); i < range.end(); i++ ) {
            if ( x-i <= 2) 
                value += 1;
            else {
                PARTITIONER partitioner;
                reduceBody reducer(x-i);
                tbb::parallel_reduce (tbb::blocked_range<int> (1,3,1), reducer, partitioner);
                value += reducer.value;
            }
        }
    }

    // Join Operation
    void join (reduceBody &rhs) { value += rhs.value; }

};

long ParallelFibReducer (long n) { 
    if ( n <= 2 )
        return 1;
    else {
        PARTITIONER partitioner;
        reduceBody reducer(n);
        tbb::parallel_reduce (tbb::blocked_range<int>(1,3,1), reducer, partitioner);
        return reducer.value;
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
    int defth = tbb::task_scheduler_init::default_num_threads();
    if (nth<0) nth=defth;
    printf("Default #Threads=%d. Using %d threads\n", defth, nth);
    tbb::task_scheduler_init init(nth);


    int j,k;
    PARTITIONER partitioner;
    tbb::tick_count t0 = tbb::tick_count::now();

    long result;
    result = ParallelFibReducer( NUMBER );
    // result = ParallelFib( NUMBER );
    // result = SerialFib( NUMBER );

    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

    printf("Result Fib(%d) = %ld\n", NUMBER, result);
    // Dump result
    int *res = (int*)&result;
    hexdump_var("result.hex", res, sizeof(long)/sizeof(int));

    return 0;
}

