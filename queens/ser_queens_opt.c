#include "tbb/tick_count.h"
#include <stdlib.h>
#include <stdio.h>

#include "../defs.h"

#ifndef TIMES
#define TIMES 1
#endif


#ifdef DEBUG
int addok_error = 0;
int addok_error_amount = 0;
#endif

#ifndef SIZE
    #define SIZE 4
#endif

#ifndef NULL
#define NULL 0
#endif

int solution[SIZE];
int solution_count = 0; // implicitly initialized to 0

/**
 * Return 0 if adding a queen at (i,j) results in an invalid chessboard
 * and 1 otherwise 
 */
int addok(int solution[SIZE], int i, int j) {
    int i2, j2;
    if (j==0) return 1;
    else for (int j2=0;j2<j; j2++) {
        i2 = solution[j2];
        if (i == i2 || j == j2 || i == i2 - (j - j2) || i == i2 + (j - j2))
                    return 0;
    }
#ifdef DEBUG
    if (j2!=-1) {
        addok_error++;
        addok_error_amount += j2;
    }
#endif
    return 1;
}

void nqueens_rec(int column, int solution[SIZE] ) {
    int i;
    for (i=0; i<SIZE; i++) {
        if (addok(solution, i, column)) {
            solution[column] = i;
            if (column+1<SIZE) {
                nqueens_rec(column+1, solution);
            } else { // found a solution 
                solution_count++;
            }
        }
        // else do nothing -- dead computation branch
    }
}

int nqueens() {
    nqueens_rec(0, solution);
    return 0;
}

int main(void) {
    int i;
    tbb::tick_count t0 = tbb::tick_count::now();
#ifdef SENS_TIMES
  for(int st=0; st<SENS_TIMES; st++) { 
#endif
    for (int c=0; c<TIMES; c++) {
        solution_count = 0;
        nqueens();
    }
#ifdef SENS_TIMES
  }
#endif
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());
    printf("NrSolutions = %d\n", solution_count);

    // Dump result
    hexdump_var("result.hex", &solution_count, 1);

#ifdef DEBUG
    printf ("addok_error : %d\n", addok_error);
    printf ("addok_error amount : %d\n", addok_error_amount);
#endif
#ifdef PRINT_RESULT
    printf("Solution Count = %d\n", solution_count);
    if (solution_count == 0) printf ("No solution found\n");
    else for(int j=0; j<SIZE; j++) {
        for(i=0;i<SIZE;i++) {
            if (i==solution[j]) printf ("*");
            else printf(" ");
        }
        printf("\n");
    }
#endif //PRINT_RESULT
    return 0;
}
