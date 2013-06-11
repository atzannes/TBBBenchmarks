/********************************************************************/
/* TSP: The Travelling Salesman Problem                             */
/* If PARTIAL_WEIGHT_CUTOFF is defined then the weight of a partial */
/* solution is used to cut computation branches that will not lead  */
/* to an optimal solution                                           */
#include "../defs.h"

#ifndef TIMES
#define TIMES 1
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef INIT
int weights[SIZE][SIZE];
#endif 

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

#define MAX_INT 0x7fffffff
int solution_weight; // = MAX_INT; but we initialize it in main to be able to
                     // vardump it
int solution[SIZE];

int partial_solution[SIZE];

void solve_tsp_rec(int A[SIZE][SIZE], int partial_solution[SIZE], int position, int partial_weight) 
{
    int i,j;
    int used;
    int myweight;
#ifdef DEBUG
    printf("Calling tsp_rec with pos=%d and partial_weight=%d\n", position, partial_weight);
#endif
#ifdef PARTIAL_WEIGHT_CUTOFF
    if (partial_weight >= solution_weight) return;
#endif
    for(i=0; i<SIZE; i++) {
        // 1. check if i was already used in the solution so far.
        //    if so, skip it.
        used = 0;
        for(j=0; j<position; j++) {
            if (partial_solution[j]==i) {
                used = 1;
                break;
             }
        }
        if (used) continue;

        partial_solution[position] = i;
        myweight = partial_weight + A[partial_solution[position-1]][i];
        if (position==SIZE-1) {
            // 2a. termination 
            myweight += A[i][0];
#ifdef DEBUG
            for(j=0; j<SIZE; j++) {
                printf("%d,", partial_solution[j]);
            }
            printf("   weight = %d\n\n", myweight);
#endif
            if (myweight < solution_weight) {
                solution_weight = myweight;
                // Save solution
                for(j=0; j<SIZE; j++) solution[j] = partial_solution[j];
            }
        } else {
            // 2b. recursion
            solve_tsp_rec(A, partial_solution, position+1, myweight);
        }
    }
}

void solve_tsp(int A[SIZE][SIZE]){
    partial_solution[0] = 0;
    solve_tsp_rec(A, partial_solution, 1, 0);
}

int main (void)
{
#ifdef INIT
    init_weights();
#endif

    tbb::tick_count t0 = tbb::tick_count::now();
    for (int c=0; c<TIMES; c++) {
        solution_weight = MAX_INT;
        solve_tsp(weights);
    }
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());
    printf("Solution Weight = %d\n", solution_weight);

    // Dump result
    hexdump_var("result.hex", &solution_weight, 1);

#ifdef PRINT_RESULT
    printf("Solution weight = %d\n", solution_weight);
    int i;
    for(i=0;i<SIZE;i++) { 
        printf("solution[%d] = %d\n", i, solution[i]);
    }
#endif
    return 0;
}
