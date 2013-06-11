/**
 *   This implementation is messy because I was trying multiple alternatives
 *  1. NO_LOCKS: instead of using locks to atomically update a single 
 *     'current_best' solution, we have a current_best solution per TCU. At 
 *     the end (in serial) we pick the best one. It seems that using a lock 
 *     works better. Perhaps having a single 'current_best' (which is used 
 *     to prune computation branches with partial solutions that exceed the 
 *     current_best) leads to more pruning which offsets the locking overhead.
 *     Perhaps the best strategy is one with a small number of locks (and 
 *     current_best solutions (i.e. a hybrid), because the single lock will
 *     not scale with more TCUs.
 *  2. DOUBLE_CHECK: I makes sense to check if a solution we found is better than
 *     the current_best before trying to lock the current_best to update it. It 
 *     seems however that this double checking causes performance degradation, 
 *     especially when combined with continuous_prefetching. This is counter-
 *     intuitive and I have no idea why it's happening
 *  3. INIT: to run TSP on a dataset size for which we don't have .h/.xbo 
 *     files. The drawback is that the initialization of the weights array
 *     is included in the computation, and the array is very regular:
 *     w(i,j)=(i-j)*(i-j)
 *  4. CUTOFF: if defined then parallelism is cut-off beyond a certain level
 *     determined by CUTOFF_LEVEL
 */
#include "../defs.h"

#ifndef SIZE
#define SIZE 10
#endif 

#ifndef GRAINSIZE
#define GRAINSIZE 1
#endif

#ifdef CUTOFF
  #ifndef CUTOFF_LEVEL
    #define CUTOFF_LEVEL (SIZE/2)
  #endif
#endif

#ifndef NULL
#define NULL 0
#endif 


#ifdef DEBUG
tbb::atomic<int> out_of_memory = 0;
tbb::atomic<int> release_failed = 0;
#endif

typedef struct list_node_tag { int val; struct list_node_tag *next;} list_node;

typedef struct {int size; list_node *head;} list;

#define MAX_INT 0x7fffffff

#ifdef NO_LOCKS
tbb::combinable<int> my_weight(MAX_INT);
#endif

#ifdef INIT
int weights[SIZE][SIZE];

void init_weights (void) {
    int i,j;
    for(i=0;i<SIZE;i++) {
        for(j=0;j<SIZE;j++) {
            if (i>j)
                weights[i][j] = (i-j)*(i-j);
            else
                weights[i][j] = (j-i)*(j-i);
        }
    }
}
#endif


volatile int solution_weight;
//tbb::atomic<int> solution_weight;
tbb::mutex solution_lock;

void ser_solve_tsp_rec(const int (*A)[SIZE], list_node* partial_solution, int position, int partial_weight) 
{

#ifdef PARTIAL_WEIGHT_CUTOFF 
#ifdef NO_LOCKS
    if (partial_weight >= my_weight.local() ) return;
#else
    if (partial_weight >= solution_weight) return;
#endif
#endif

    int i;
    for(i=0; i<SIZE; i++){
        // 1. check if i was already used in the solution so far.
        //    if so, skip it.
        int used = 0;
        int j = position;
        list_node* tmp_p = partial_solution;
               
        while(tmp_p != NULL) {
            if (tmp_p->val == i) { 
                used = 1;
                break;
            }
            tmp_p = tmp_p->next;
        }

        if (!used) {
            //partial_solution[position] = i;
            // allocate a node
            list_node new_node;
            new_node.next = partial_solution;
            new_node.val = i;
            
            int myweight = partial_weight + A[partial_solution->val][i];
            if (position==SIZE-1) {
                // 2a. termination 
#ifdef NO_LOCKS
                myweight += A[i][0];
                if (myweight < my_weight.local() ) {
                    my_weight.local() = myweight;
                }
#else
                myweight += A[i][0];

#ifdef DOUBLE_CHECK
                if (myweight < solution_weight) {
#endif
                    // acquire lock
                    tbb::mutex::scoped_lock myLock(solution_lock);
                    if (myweight < solution_weight) {
                        // update solution
                        solution_weight = myweight;
                    }
                    // implicit lock release
#ifdef DOUBLE_CHECK
                }
#endif
#endif

            } else {
                // 2b. recursion
                ser_solve_tsp_rec(A, &new_node, position+1, myweight);
            }
        } // end if(!used)
    } // end spawn
}

void solve_tsp_rec(const int (*A)[SIZE], list_node* partial_solution, int position, int partial_weight);


class tspMeldFlatBody {
public:
    //int* A[SIZE];
    const int (*A)[SIZE];
    list_node* partial_solution;
    int position;
    int partial_weight;

    void operator () (tbb::blocked_range<int> &range, tbb::task* parent_task) const {

        for (int i=range.begin(); i<range.end() /*&& parent_task->bflws_pushed==false*/; i++) {
            range.remove_some_safe();
            // 1. check if node i was already used in the solution so far.
            //    if so, skip it.
            int used = 0;
            int j = position;
            list_node* tmp_p = partial_solution;
               
            while(tmp_p != NULL) {
                if (tmp_p->val == i) { 
                    used = 1;
                    break;
                }
                tmp_p = tmp_p->next;
            }
 
            if (!used) {
                //partial_solution[position] = i;
                // allocate a node
                list_node new_node;
                tmp_p = &new_node;

                int myweight;
                tmp_p->next = partial_solution;
                tmp_p->val  = i;
              
                myweight = partial_weight + A[partial_solution->val][i];
                if (position==SIZE-1) {
                    // 2a. termination 
#ifdef NO_LOCKS
                    myweight += A[i][0];
                    if (myweight < my_weight.local() ) {
                        my_weight.local() = myweight;
                    }
#else
                    myweight += A[i][0];
#ifdef DOUBLE_CHECK
                    if (myweight < solution_weight) {
#endif 
                        tbb::mutex::scoped_lock myLock(solution_lock);
                        if (myweight < solution_weight) {
                            // lock solution
                            solution_weight = myweight;
                        }
                        // implicit lock release
#ifdef DOUBLE_CHECK
                    }
#endif
#endif

                } else {
                    // 2b. recursion
#ifdef CUTOFF
                    if (position+1>=CUTOFF_LEVEL) 
                        ser_solve_tsp_rec(A, tmp_p, position+1, myweight);
                    else
                        solve_tsp_rec(A, tmp_p, position+1, myweight);
#else
                    solve_tsp_rec(A, tmp_p, position+1, myweight);
#endif
                }
            } // end if(!used)
            DEQUE_CHECK_CODE (parent_task);
        } // end for loop
    }


    // Constructor
    tspMeldFlatBody(const int A[SIZE][SIZE], list_node* partial_solution, int position, int partial_weight):
        A(A), partial_solution(partial_solution), position(position), partial_weight(partial_weight) {}


};


void solve_tsp_rec(const int (*A)[SIZE], list_node* partial_solution, int position, int partial_weight) 
{
#ifdef PARTIAL_WEIGHT_CUTOFF 
#ifdef NO_LOCKS
    if (partial_weight >= my_weight.local() ) return;
#else
    if (partial_weight >= solution_weight) return;
#endif
#endif
    PARTITIONER partitioner;
    tbb::parallel_for (tbb::blocked_range<int>(0,SIZE,GRAINSIZE), tspMeldFlatBody(A,partial_solution,position,partial_weight), partitioner);
}

void solve_tsp(int A[SIZE][SIZE]){
    // pick the 1st node of the tour
    list_node tmp_p;
    tmp_p.next = NULL;
    tmp_p.val  = 0;
    
    solve_tsp_rec(A, &tmp_p, 1, 0);
}

int min(int x, int y) {
   return (x<y) ? x : y;
}

template <typename T>
struct FunctorMin {
    T operator () (T left, T right) { return (left<right) ? left : right;}
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

    // init lock
#ifdef DEBUG_VERBOSE
    printf("test1\n");
#endif

#ifdef INIT
    init_weights();
#endif
#ifdef DEBUG_VERBOSE
    printf("release_failed = %d\n", release_failed);
    printf("out_of_memory  = %d\n", out_of_memory);
    printf("test2\n");
#endif
    tbb::tick_count t0 = tbb::tick_count::now();
    solution_weight = MAX_INT;
    solve_tsp(weights);
#ifdef NO_LOCKS
    //FunctorMin<int> my_min;
    solution_weight = my_weight.combine (min);
    //solution_weight = my_weight.combine ( my_min);
    //solution_weight = my_weight.combine ( [](int x, int y){return (x<y) ? x : y; });
#endif
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());
    printf("Solution Weight = %d\n", solution_weight);

#ifdef DEBUG_VERBOSE
    printf("release_failed = %d\n", release_failed);
    printf("out_of_memory  = %d\n", out_of_memory);
#endif
#ifdef DEBUG
    // if there is an error invalidate solution_weight
    if (release_failed || out_of_memory) solution_weight = -1;
#endif 
#ifdef PRINT_RESULT
    printf("Solution weight = %d\n", solution_weight);
    list_node* tmp_p = solution;
    int i=SIZE;
    while(tmp_p != NULL && i!=0) {
        i--;
        printf("solution[%d] = %d\n", i, tmp_p->val);
        tmp_p = tmp_p->next;
    }
#endif
    return 0;
}
