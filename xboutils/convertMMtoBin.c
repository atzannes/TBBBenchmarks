#include "readMMformat.c"
#include <stdio.h>
#include <stdlib.h>

#define N_ARGS 4
int main(int argc, char *argv[]) {
  if (argc!=N_ARGS) {
    printf("ERROR: wrong number of arguments (%d instead of %d)\n", argc, N_ARGS);
    printf("Usage: %s NSeps MMFILE BINFILE\n", argv[0]);
    exit(1);
  }
  int NSeps = atoi(argv[1]);
  assert(NSeps>=2);
  char *mm_file = argv[2];
  char *bin_file = argv[3];
  convertMMtoBinFormat(mm_file, NSeps, bin_file);

  return 0;
}
