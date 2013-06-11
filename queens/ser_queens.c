#include "../defs.h"

#ifdef DEBUG
int addok_error = 0;
int addok_error_amount = 0;
int out_of_memory = 0;
#endif

#ifndef SIZE
    #define SIZE 4
#endif

#ifndef NULL
#define NULL 0
#endif

typedef struct list_node_tag { int row; struct list_node_tag *next;} list_node;

typedef struct {int size; list_node *head;} list;


list_node *solution;
int solution_count; // implicitly initialized to 0

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
        //int tmp = 1;
        //psm(tmp,addok_error);
        addok_error++;
        //psm(j2,addok_error_amount);
        addok_error_amount += j2;
    }
#endif
    return 1;
}

void nqueens_rec(int column, list_node* head) {
    int i;
    for (i=0; i<SIZE; i++) {
        list_node *node;
        if (addok(head, i, column)) {
            // add the node
            // node = malloc_list_node();
            node = (list_node*)malloc(sizeof(list_node));
#ifdef DEBUG
            if(node==NULL) out_of_memory = 1;
#endif
#ifdef QUIT_ON_SOLUTION // QUIT as soon as a solution is found
            if (node!=NULL && solution == NULL) {
#else                   // Don't quit. Instead keep scanning the whole search space.
            if (node!=NULL) {
#endif
                node->next = head;
                node->row  = i;
                if (column+1<SIZE) {
                    nqueens_rec(column+1, node);
                } else { // found a solution 
                  solution = node;
                  solution_count++;
                  //abort()
                }
            }
        }
        // else do nothing -- dead computation branch
    }
}

list_node* nqueens() {
    nqueens_rec(0, NULL);
    return solution;    
}

int main(void) {
    int i;
    solution = NULL;
#ifdef DEBUG
    printf ("addok_error : %d\n", addok_error);
    printf ("addok_error amount : %d\n", addok_error_amount);
#endif
    tbb::tick_count t0 = tbb::tick_count::now();
    for (int c=0; c<TIMES; c++) {
        solution_count = 0;
        solution = nqueens();
    }
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());
    printf("NrSolutions = %d\n", solution_count);

    // Dump result
    hexdump_var("result.hex", &solution_count, 1);

#ifdef DEBUG
    if (out_of_memory || addok_error) solution_count = -1;
#endif

#ifdef DEBUG
    printf("out_of_memory errors: %d\n", out_of_memory);
    //printf ("grLO: %d, grHI: %d\n");
    printf ("addok_error : %d\n", addok_error);
    printf ("addok_error amount : %d\n", addok_error_amount);
#endif
#ifdef PRINT_RESULT
    if (solution == NULL) printf ("No solution found\n");
    else if (solution == -2) printf ("solution not updated\n");
    else if (solution == -1) printf ("Ran out of memory to allocate\n");
    else while (solution!=NULL) {
        for(i=0;i<SIZE;i++) {
            if (i==solution->row) printf ("*");
            else printf(" ");
        }
        printf("\n");
        solution = solution->next;
    }
#endif //PRINT_RESULT
    return 0;
}
