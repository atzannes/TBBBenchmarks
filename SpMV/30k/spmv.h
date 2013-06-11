/****************************************
 Automatically Generated Header File


          DO NOT MODIFY 

 Contains following Variables : 

Name      : rowptr
Type      : int
Dimension : 1
Size [0]  : 30001
Source    : matrixRowPtr.txt

Name      : col_ind
Type      : int
Dimension : 1
Size [0]  : 60130
Source    : matrixCols.txt

Name      : values
Type      : int
Dimension : 1
Size [0]  : 60130
Source    : matrixValues.txt

Name      : vector
Type      : int
Dimension : 1
Size [0]  : 100
Source    : vectorValues.txt

Name      : result
Type      : int
Dimension : 1
Size [0]  : 30000
Source    : 0

****************************************/
#define SIZE_M 30000
#define SIZE_N 100
#define NUMBER_NON_ZERO 60130

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


