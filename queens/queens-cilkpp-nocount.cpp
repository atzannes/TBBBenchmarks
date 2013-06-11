/**
 *  1. CUTOFF: if defined parallelism is cut off after a certain level 
 *     (depth) which is determined by CUTOFF_LEVEL
 *  2. QUIT_ON_SOLUTION: when defined computation stops (soon after)
 *     a solution is found. If it's not defined all solutions will be
 *     found
 *  In this version we disable the cilk_reducer to measure cilk's performance
 *  without it because we noticed that the tuned version of queens (cutoff=N/2)
 *  performs worse with cilk than with TBB (20X vs 18X) and we want to see if
 *  the performance of the cilk reducer might be at fault.
 */

#ifdef CUTOFF
  #ifndef CUTOFF_LEVEL
    #define CUTOFF_LEVEL 5
  #endif
#endif

#include <cilk.h>
#include <reducer_opadd.h>
#include <stdio.h>
#include "../xboutils/xboutils.h"
#include "tbb/tick_count.h"

//cilk::reducer_opadd<int> solution_count;

#ifdef DEBUG
psBaseReg threadCount;
int addok_error = 0;
int addok_error_amount = 0;
#endif

#ifndef SIZE
    #define SIZE 4
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

/*****************************************/

//list_node *solution;

#ifndef SEPARATE_ADDOK
inline
#endif
int addok(list_node *head, int i, int j) {
    int i2, j2=j-1;
    if (head==NULL && j==0) return 1;
    else while (head!=0) {
        i2 = head->row;
        if (i == i2 || j == j2 || i == i2 - (j - j2) || i == i2 + (j - j2))
                    return 0;
        head = head->next;
        j2--;
    }
#ifdef DEBUG
    if (j2!=-1) {
        int tmp = 1;
        psm(tmp,addok_error);
        psm(j2,addok_error_amount);
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
            list_node new_node;
            new_node.next = head;
            new_node.row  = i;
            if (column+1<SIZE) {
                ser_nqueens_rec(column+1, &new_node);
            } else { // found a solution 
              //solution_count += 1;
            }
        }
        // else do nothing -- dead computation branch
    }
}

//#define cilk_for for

void nqueens_rec(int column, list_node* head) {

    cilk_for (int i=0; i<SIZE; i++) {
        if (addok(head, i, column)) {
            // add the node
            list_node new_node;
            new_node.next = head;
            new_node.row  = i;

            if (column+1<SIZE) {
#ifdef CUTOFF
                if (column+1>=CUTOFF_LEVEL)
                    ser_nqueens_rec(column+1, &new_node);
                else
                    nqueens_rec(column+1, &new_node);
#else
                nqueens_rec(column+1, &new_node);
#endif
            } else { // found a solution 
                //solution_count += 1;
            }
        }
        // else do nothing -- dead computation branch
    }
}

void nqueens() {
    //solution_count.set_value(0);
    nqueens_rec(0, NULL);
}

int cilk_main(void) {
    int i;
    //solution = NULL;
#ifdef DEBUG_VERBOSE
    threadCount = 0;
    printf ("addok_error : %d\n", addok_error);
    printf ("addok_error amount : %d\n", addok_error_amount);
    printf ("initial global_sp = %d\n", __xmt_global_sp);
#endif
    tbb::tick_count t0 = tbb::tick_count::now();
#ifdef SENS_TIMES
  for (int st=0; st<SENS_TIMES; st++) {
#endif
    for (int c=0; c<TIMES; c++)
        nqueens();
#ifdef SENS_TIMES
  }
#endif
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds() );
    int count = 0; // solution_count.get_value();
    printf("NrSolutions = %d\n", count);

    // Dump result
    hexdump_var("result.hex", &count, 1);

#ifdef DEBUG
    if (out_of_memory || addok_error) solution_count = -1;
#endif
#ifdef DEBUG_VERBOSE
    printf ("final global_sp = %d\n", __xmt_global_sp);
    //printf ("grLO: %d, grHI: %d\n");
    printf ("addok_error : %d\n", addok_error);
    printf ("addok_error amount : %d\n", addok_error_amount);
#endif
#ifdef MEM_FOOTPRINT
    printf("Gl_SP=%d\n", __xmt_global_sp);
#endif
#ifdef PRINT_RESULT
    list_node *tmp = solution;
    if (tmp == NULL) printf ("No solution found\n");
    else if (tmp == -2) printf ("solution not updated\n");
    else if (tmp == -1) printf ("Ran out of memory to allocate\n");
    else while (tmp!=NULL) {
        for(i=0;i<N;i++) {
            if (i==tmp->row) printf ("*");
            else printf(" ");
        }
        printf("\n");
        tmp = tmp->next;
    }
#endif //PRINT_RESULT
#ifdef CHECK_RESULT
    list_node *tmpP = solution;
    for(i=0;i<N && tmpP!=NULL;i++, tmpP=tmpP->next) {
        int row, column, diag1, diag2;
        row = tmpP->row;
        column = i;
        diag1 = row+column;
        diag2 = N + row - column - 1;
        //printf("Setting rowAr[%d] to %d\n", row, rowAr[row]+1);
        rowAr[row]++;
        columnAr[column]++;
        diagonal1Ar[diag1]++;
        diagonal2Ar[diag2]++;
    }
    printf("i=%d\n", i );
    printf("Rows with incorrect number of queens:");
    for(i=0; i<N; i++) {
        if (rowAr[i]!=1) printf("%d=%d,", i,rowAr[i]);
    }
    printf("\n");

    printf("Columns with incorrect number of queens:");
    for(i=0; i<N; i++) {
        if (columnAr[i]!=1) printf("%d,", i);
    }
    printf("\n");

    printf("Diagonal1 with more than 1 queens:");
    for(i=0;i<2*N-1; i++) { 
        if(diagonal1Ar[i] > 1) printf("%d,", i);
    }
    printf("\n");

    printf("Diagonal2 with more than 1 queens:");
    for(i=0;i<2*N-1; i++) { 
        if(diagonal2Ar[i] > 1) printf("%d,", i);
    }
    printf("\n");

#endif // CHECK_RESULT
    return 0;
}

