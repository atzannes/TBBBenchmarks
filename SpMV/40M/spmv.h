/****************************************
 Automatically Generated Header File


          DO NOT MODIFY 

 Contains following Variables : 

Name      : rowptr
Type      : int
Dimension : 1
Size [0]  : 80001
Source    : rowPtr.txt

Name      : col_ind
Type      : int
Dimension : 1
Size [0]  : 40007690
Source    : cols.txt

Name      : values
Type      : int
Dimension : 1
Size [0]  : 40007690
Source    : values.txt

Name      : vector
Type      : int
Dimension : 1
Size [0]  : 5000
Source    : R 10000

Name      : result
Type      : int
Dimension : 1
Size [0]  : 80000
Source    : 0

****************************************/
#define SIZE_M 80000
#define SIZE_N 5000
#define NUMBER_NON_ZERO 40007690

extern int    rowptr[SIZE_M+1];
extern int rowptr_dim0_size;

extern int    col_ind[NUMBER_NON_ZERO];
extern int col_ind_dim0_size;

extern int    values[NUMBER_NON_ZERO];
extern int values_dim0_size;

extern int    vector[SIZE_N];
extern int vector_dim0_size;

extern int    result[SIZE_M];
extern int result_dim0_size;

