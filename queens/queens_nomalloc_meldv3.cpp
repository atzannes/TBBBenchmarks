/**
 *  1. CUTOFF: if defined parallelism is cut off after a certain level 
 *     (depth) which is determined by CUTOFF_LEVEL
 *  2. QUIT_ON_SOLUTION: when defined computation stops (soon after)
 *     a solution is found. If it's not defined all solutions will be
 *     found
 */
#include "../defs.h"

#ifndef TIMES
#define TIMES 1
#endif

#ifndef GRAINSIZE
#define GRAINSIZE 1
#endif 

#ifdef CUTOFF
  #ifndef CUTOFF_LEVEL
    #define CUTOFF_LEVEL (SIZE/2)
  #endif
#endif


#ifdef DEBUG
tbb::atomic<int> threadCount;
tbb::atomic<int> addok_error = 0;
tbb::atomic<int> addok_error_amount = 0;
int out_of_memory = 0;
#endif

#ifndef SIZE
    #define SIZE 9
#endif

#ifdef CHECK_RESULT
    int rowAr[SIZE];
    int columnAr[SIZE];
    int diagonal1Ar[2*SIZE-1];
    int diagonal2Ar[2*SIZE-1];
#endif // CHECK_RESULT

#ifndef NULL
#define NULL 0
#endif

// The row information is explicit, but the column is implicit by the position in the list
typedef struct list_node_tag { int row; struct list_node_tag *next;} list_node;

typedef struct {int size; list_node *head;} list;


//list_node *solution;
tbb::atomic<int> solution_count; // implicitly initialized to 0

/**
 * Takes a list of positions l and a position p and returns true(1) if
 * a queen at position p will not attack any of the positions in the list l.
 * and false(0) otherwise.
*/
int addok(list_node *head, int i, int j) {
    // i2 and j2 are the coordinates of the position compared to position p(i,j)
    int i2, j2=j-1; 
                    
    if (head==NULL && j==0) return 1;

    else while (head!=NULL) {
        i2 = head->row;
        if (  i == i2          // same row
           || j == j2          // same column 
           || i+j == i2+j2     // same diagonal 
           || i-j == i2-j2 )   // same diagonal
                    return 0;
        head = head->next;
        j2--;
    }
#ifdef DEBUG
    if (j2!=-1) {
        int tmp = 1;
        addok_error++; // atomic increment
        addok_error_amount += j2; // atomic
    }
#endif
    return 1;
}



void ser_nqueens_rec(int column, list_node* head) {
    int i;
    for (i=0; i<SIZE; i++) {
        list_node *node;
        if (addok(head, i, column)) {
            // add the node

#ifdef QUIT_ON_SOLUTION // QUIT as soon as a solution is found
            if (solution == NULL) {
#endif
                list_node new_node;
                new_node.next = head;
                new_node.row = i;
                if (column+1<SIZE) {
                    ser_nqueens_rec(column+1, &new_node);
                } else { // found a solution 
                  //solution = &new_node;
                  solution_count++; // atomic
                }
#ifdef QUIT_ON_SOLUTION // QUIT as soon as a solution is found
            }
#endif
        }
        // else do nothing -- dead computation branch
    }// end if addok
}

void nqueens_rec(int column, list_node* head, tbb::task* parent_task);

class nqueensMeldBody {
public:
    int column;
    list_node* head;

    void operator () (tbb::blocked_range<int> &range, tbb::task* parent_task) const {

        tbb::blocked_range<int> subrange = range.get_some_safe();
        long savedcnt = 0;
        //while ( subrange.empty() == false && parent_task->bflws_pushed == false) {
        while ( subrange.empty() == false ) {
            for(int i=subrange.begin(); i<subrange.end(); i++) {
                list_node *node;
                if (addok(head, i, column)) {
                    // add the node

#ifdef QUIT_ON_SOLUTION // QUIT as soon as a solution is found
                    if (node!=NULL && solution == NULL) {
#endif
                        list_node new_node;
                        new_node.next = head;
                        new_node.row  = i;
                        if (column+1<SIZE) {
#ifdef CUTOFF
                            if (column+1>=CUTOFF_LEVEL)
                                ser_nqueens_rec(column+1, &new_node);
                            else
                                nqueens_rec(column+1, &new_node, parent_task);
#else
                            nqueens_rec(column+1, &new_node, parent_task);
#endif

                        } else { // found a solution 
                            //solution = &new_node;
                            solution_count++; //atomic
                            //abort()
                        }
#ifdef QUIT_ON_SOLUTION // QUIT as soon as a solution is found
                    }
#endif
                } // end if addok
            // else do nothing -- dead computation branch
            } // end for-loop over range

#ifdef DEQUE_THRESH
            if ( parent_task->task_pool_size() < DEQUE_THRESH) break;
#else
            if ( parent_task->is_task_pool_empty() ) break;
#endif

            subrange = range.get_some_safe();
            //printf("Saved one ping-pong!\n");
            //std::cout << "Saved one\n" ;
            //savedcnt ++;
        } // end while
        //printf("Saved %d ping-pongs\n", savedcnt);
        //std::cout << "Saved " << savedcnt << "\n" ;
    }

    // Constructor
    nqueensMeldBody(int col, list_node* hd) : column(col), head(hd) { }
};

class nqueensMeldFlatBody {
public:
    int column;
    list_node* head;

    void operator () (tbb::blocked_range<int> &range, tbb::task* parent_task) const {

        //int cnt = 0;
        for(int i = range.begin(); i < range.end() /*&& parent_task->bflws_pushed == false*/; i++) {
            range.remove_some_safe();
            list_node *node;
            if (addok(head, i, column)) {
                // add the node
#ifdef QUIT_ON_SOLUTION // QUIT as soon as a solution is found
                if (node!=NULL && solution == NULL) {
#endif
                    list_node new_node;
                    new_node.next = head;
                    new_node.row  = i;
                    if (column+1<SIZE) {
#ifdef CUTOFF
                        if (column+1>=CUTOFF_LEVEL)
                            ser_nqueens_rec(column+1, &new_node);
                        else
                            nqueens_rec(column+1, &new_node, parent_task);
#else
                        nqueens_rec(column+1, &new_node, parent_task);
#endif
                    } else { // found a solution 
                        //solution = &new_node;
                        solution_count++; //atomic
                        //abort()
                    }
#ifdef QUIT_ON_SOLUTION // QUIT as soon as a solution is found
                }
#endif
            } // end if addok
            // else do nothing -- dead computation branch

            //cnt++;
            //if (cnt == GRAINSIZE)) {
                //cnt = 0;
                //subrange = range.remove_some_safe();
            DEQUE_CHECK_CODE (parent_task);
                //else end = range.end();
            //}

            //printf("Saved one ping-pong!\n");
            //std::cout << "Saved one\n" ;
            //savedcnt ++;
            //i++;
        } // end for-loop over range
        //printf("Saved %d ping-pongs\n", savedcnt);
        //std::cout << "Saved " << savedcnt << "\n" ;
    }

    // Constructor
    nqueensMeldFlatBody(int col, list_node* hd) : column(col), head(hd) { }
};

//using namespace tbb::internal;
namespace tbb {

namespace internal {

void nqueens_rec(int column, list_node* head, tbb::task* parent_task) {
    PARTITIONER partitioner;
    tbb::blocked_range<int> range(0,SIZE,GRAINSIZE);
#ifdef _TBB_MELD_ALLOW_CODE_DUPLICATION
    for(int i = range.begin(); i < range.end() /*&& parent_task->bflws_pushed == false*/; i++) {
       // DEQUE CHECK
       if ( parent_task==NULL || parent_task->is_task_pool_empty() ) {
            tbb::parallel_for (range, nqueensMeldFlatBody(column,head), partitioner);
            break;
        } // else
        range.remove_some_safe();
        list_node *node;
        if (addok(head, i, column)) {
            // add the node
#ifdef QUIT_ON_SOLUTION // QUIT as soon as a solution is found
            if (node!=NULL && solution == NULL) {
#endif
                list_node new_node;
                new_node.next = head;
                new_node.row  = i;
                if (column+1<SIZE) {
#ifdef CUTOFF
                    if (column+1>=CUTOFF_LEVEL)
                        ser_nqueens_rec(column+1, &new_node);
                    else
                        nqueens_rec(column+1, &new_node, parent_task);
#else
                    nqueens_rec(column+1, &new_node, parent_task);
#endif
                } else { // found a solution 
                    //solution = &new_node;
                    solution_count++; //atomic
                    //abort()
                }
#ifdef QUIT_ON_SOLUTION // QUIT as soon as a solution is found
            }
#endif
        } // end if addok
    } // end for-loop over range
#else  // TBB_MELD_DONT_ALLOW_CODE_DUPLICATION
    nqueensMeldFlatBody body(column,head);
    // create task
    //start_for<tbb::blocked_range<int>, nqueensMeldFlatBody, PARTITIONER> *self = NULL;
    //self = new(task::allocate_root()) start_for(range,body,const_cast<PARTITIONER&>(partitioner));
    start_for<tbb::blocked_range<int>, nqueensMeldFlatBody, PARTITIONER>& a = *new(task::allocate_root()) start_for(range,body,const_cast<Partitioner&>(partitioner));

    // add to postponed deque
    self->next_postponed = NULL;
    // enqueue postponed task
    tbb::task* saved_tail = self->push_postponed_task();

    do {
        //if ( parent_task==NULL || parent_task->is_task_pool_empty() ) {
        if ( self->is_task_pool_empty() ) {
            // TODO: optim if possible split ancestor postponed and continue:
            // note: this will always be possible since we already added it to
            // the postponed deque.
            parallel_for (range, body, partitioner);
            break;
        } // else
        body(range,parent_task);
    } while(range.empty()==false);
    // TODO detect if a split has occurred, in which case some synchronization is needed
#endif
}

} // namespace internal

} // namespace tbb

int nqueens() {
    solution_count = 0;
    nqueens_rec(0, NULL, NULL);
    return solution_count;    
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

    int i;
    //solution = NULL;
#ifdef DEBUG_VERBOSE
    threadCount = 0;
    printf ("addok_error : %d\n", addok_error);
    printf ("addok_error amount : %d\n", addok_error_amount);
    printf ("initial global_sp = %d\n", __xmt_global_sp);
#endif
    tbb::tick_count t0 = tbb::tick_count::now();
    for (int c=0; c<TIMES; c++)
        nqueens();
    tbb::tick_count t1 = tbb::tick_count::now();
    int count = solution_count;
    printf("Ticks = %f\n", (t1-t0).seconds() );
    printf("NrSolutions = %d\n", count);

    // Dump result
    hexdump_var("result.hex", &count, 1);

#ifdef DEBUG
    if (out_of_memory || addok_error) solution_count = -1;
#endif
#ifdef DEBUG_VERBOSE
    printf ("addok_error : %d\n", addok_error);
    printf ("addok_error amount : %d\n", addok_error_amount);
#endif
#ifdef PRINT_RESULT
    /*list_node *tmp = solution;
    if (tmp == NULL) printf ("No solution found\n");
    else if (tmp == -2) printf ("solution not updated\n");
    else if (tmp == -1) printf ("Ran out of memory to allocate\n");
    else while (tmp!=NULL) {
        for(i=0;i<SIZE;i++) {
            if (i==tmp->row) printf ("*");
            else printf(" ");
        }
        printf("\n");
        tmp = tmp->next;
    }*/
#endif //PRINT_RESULT
#ifdef CHECK_RESULT
    /*list_node *tmpP = solution;
    for(i=0;i<SIZE && tmpP!=SIZEULL;i++, tmpP=tmpP->next) {
        int row, column, diag1, diag2;
        row = tmpP->row;
        column = i;
        diag1 = row+column;
        diag2 = SIZE + row - column - 1;
        //printf("Setting rowAr[%d] to %d\n", row, rowAr[row]+1);
        rowAr[row]++;
        columnAr[column]++;
        diagonal1Ar[diag1]++;
        diagonal2Ar[diag2]++;
    }
    printf("i=%d\n", i );
    printf("Rows with incorrect number of queens:");
    for(i=0; i<SIZE; i++) {
        if (rowAr[i]!=1) printf("%d=%d,", i,rowAr[i]);
    }
    printf("\n");

    printf("Columns with incorrect number of queens:");
    for(i=0; i<SIZE; i++) {
        if (columnAr[i]!=1) printf("%d,", i);
    }
    printf("\n");

    printf("Diagonal1 with more than 1 queens:");
    for(i=0;i<2*SIZE-1; i++) { 
        if(diagonal1Ar[i] > 1) printf("%d,", i);
    }
    printf("\n");

    printf("Diagonal2 with more than 1 queens:");
    for(i=0;i<2*SIZE-1; i++) { 
        if(diagonal2Ar[i] > 1) printf("%d,", i);
    }
    printf("\n");
    */
#endif // CHECK_RESULT
    return 0;
}
