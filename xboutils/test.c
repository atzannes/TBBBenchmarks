#include <stdio.h>
#include <stdlib.h>
#include "xboutils.h"


int main(int argc, char ** argv) {

  int ll; 

  char * xbof = argv[1]; 

  /** tests for ../SpMV/40M/spmv.xbo */
  ll = get_var_length_xbo(xbof, "col_ind_dim0_size"); 
  printf("Length of var col_ind_dim0_size = %d\n", ll);

  ll = get_var_offset_xbo(xbof, "col_ind_dim0_size"); 
  printf("Offset of var col_ind_dim0_size = %d\n", ll);

  read_int_data_var(xbof, &ll, "col_ind_dim0_size");
  printf("Value of col_ind_dim0_size = %d\n", ll);
  /* */

  /** tests for ../matmult/4/mat.xbo
  int offsetZ = get_var_offset_xbo(xbof, "Z");
  printf("Offset of Z in .xbo = %d\n", offsetZ);

  int Xlen = get_var_length_xbo(xbof, "X")/4;
  int offsetX = get_var_offset_xbo(xbof, "X");
  printf ("Offset of X in .xbo = %d after Z\n", offsetX-offsetZ);

  int *X = (int*)malloc(sizeof(int)*Xlen);
  read_int_data_var(xbof, X, "X");
  //read_int_data(xbof, X, offsetX-3, Xlen*4);
  printf("X length = %d\nX = ", Xlen);
  for(int i=0; i<Xlen; i++) printf("%d, ", X[i]);
  printf("\n");

  int Zlen = get_var_length_xbo(xbof, "Z")/4 ;

  int *Z = (int*)malloc(sizeof(int)*Zlen);
  read_int_data_var(xbof, Z, "Z");
  //read_int_data(xbof, Z, 0xf8-1, Zlen*4);
  printf("Z length = %d\nZ = ", Zlen);
  for(int i=0; i<Zlen; i++) printf("%d, ", Z[i]);
  printf("\n");*/

  ll = get_xbo_header_length(xbof); 
  printf("Length of header of xbo file= %d\n", ll); 

}
