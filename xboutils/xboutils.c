#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xboutils.h"

#define MAX_LINE_SIZE 512

/** Reads integer data from an input .xbo file. 
 * f_name : name of .xbo file
 * dest : destination address
 * f_offset : offset in binary file where data starts
 * length : number of ints to read   */
int 
read_int_data_var(const char * f_name, const void * dest, const char* v_name) 
{
  int offset = get_var_offset_xbo(f_name, v_name);
  int length = get_var_length_xbo(f_name, v_name);
  read_int_data(f_name, dest, offset, length);
  return 0; 
}

/** Reads integer data from an input binary file. 
 * f_name : name of binary file
 * dest : destination address
 * f_offset : offset in binary file where data starts
 * length : number of ints to read   
 * if __BIG_ENDIAN_XBO is defined the order of the bytes is shuffled for 
 * big endian machines (xmt and x86 and .xbo files are little endian)
 */
int 
read_int_data(const char * f_name, const void * dest, const int f_offset, const int length) 
{
  FILE * f;
  int no_read = 0; 
  int no_left = length; 
  int * crt_dest_int = (int *) dest; 
  char * crt_dest = (char *) dest; 
  int BLOCK_SIZE = 512; 

  f = fopen(f_name, "r"); 
  if (f == NULL) {
    fprintf(stderr, "Could not open input file: %s\nAborting!\n", f_name); 
    exit(-1); 
  }
   
   /* Seek to the desired offset */
  if (fseek(f, f_offset, SEEK_SET) != 0) {
    fprintf(stderr, "Could not seek into input file. Aborting!\n");
    exit(-1); 
  }
  while (no_left > 0) {
    int amount = (no_left>BLOCK_SIZE)? BLOCK_SIZE : no_left;

    no_read = fread(crt_dest, sizeof(char), amount, f); 

    if (no_read != amount) {
      fprintf(stderr, "Error while reading from input file. Aborting!\n"); 
      exit(-1); 
    }
    no_left -= amount; 
    crt_dest += amount; 
  }
#ifdef __BIG_ENDIAN_XBO
    int i,x;
    for (i=0; i< length/4; i++) {
        x = crt_dest_int[i];
        x =((x>>24)& 0x000000FF) | 
           ((x<<8) & 0x00FF0000) |
           ((x>>8) & 0x0000FF00) |
            (x<<24);
        crt_dest_int[i] = x;
    }
#endif     

  int closeRes = fclose(f);
  if (closeRes) {
    printf("Error closing file' %s'\n", f_name);
    return -1;
  }

  return 0; 
}

/** Return the length in bytes that the variable var_name occupies
 *  f_name: name of the xbo file to parse
 *  var_name: name of the variable to work ok
 */
int 
get_var_length_xbo(const char * f_name, const char * var_name) 
{

  int not_done = 1; 
  int retval = 0; 

  char  buf[MAX_LINE_SIZE]; 
  FILE * f;
  char * token; 
  char * delim = (char*) " \t,";

  f = fopen(f_name, "r"); 
  if (f == NULL) {
    fprintf(stderr, "Could not open input file: %s\nAborting!\n", f_name); 
    exit(-1); 
  }

  while (not_done) {
    fgets(buf, MAX_LINE_SIZE-1, f); 
#ifdef DEBUG
    printf("Line read: [%s]\n", buf); 
#endif

    token = strtok(buf, delim); 
    if (strcmp(token, ".comm") == 0) {
      /* We have a .comm directive - meaning there is a variable here */
      token = strtok(NULL, delim); 
#ifdef DEBUG
      printf("Found variable name: [%s]\n", token); 
#endif

      if (strcmp(token, var_name) == 0) {
	/* We found the variable we're looking for */
	token = strtok(NULL, delim); 
	retval = atoi(token); 
	not_done = 0; 
      }
    }
    

#ifdef DEBUG 
    printf("First token: [%s]\n", token); 
#endif

    if (strcmp(token, ".ident") == 0)
      not_done = 0;

  } // while not_done

  int closeRes = fclose(f);
  if (closeRes) {
    printf("Error closing file' %s'\n", f_name);
    return -1;
  }
#ifdef DEBUG 
  printf("Returning value: %d\n", retval);
#endif
  return retval; 
}

/** Return the offset in bytes where the variable var_name starts
 *  f_name: name of the xbo file to parse
 *  var_name: name of the variable to work ok
 */
int 
get_var_offset_xbo(const char * f_name, const char * var_name) 
{

  int not_done = 1; 
  int retval = get_xbo_header_length(f_name); 

  char  buf[MAX_LINE_SIZE]; 
  FILE * f;
  char * token; 
  char * delim =  (char*)" \t,";

  f = fopen(f_name, "r"); 
  if (f == NULL) {
    fprintf(stderr, "Could not open input file: %s\nAborting!\n", f_name); 
    exit(-1); 
  }

  while (not_done) {
    fgets(buf, MAX_LINE_SIZE-1, f); 
#ifdef DEBUG
    printf("Line read: [%s]\n", buf); 
#endif

    token = strtok(buf, delim); 
    if (strcmp(token, ".comm") == 0) {
      /* We have a .comm directive - meaning there is a variable here */
      token = strtok(NULL, delim); 
#ifdef DEBUG
      printf("Found variable name: [%s]\n", token); 
#endif

      if (strcmp(token, var_name) != 0) {
	/* We found a variable before the one we are interested in */
	token = strtok(NULL, delim); 
	retval += atoi(token); 
      } else {
	not_done = 0; 
      }
    }
    

#ifdef DEBUG 
    printf("First token: [%s]\n", token); 
#endif

    /* .ident denotes the end of the .xbo header */
    if (strcmp(token, ".ident") == 0) 
      not_done = 0;

  } // while not_done

  int closeRes = fclose(f);
  if (closeRes) {
    printf("Error closing file' %s'\n", f_name);
    return -1;
  }

#ifdef DEBUG 
  printf("Returning value: %d\n", retval);
#endif
  return retval; 
}

/* Returns the length of the header of an .xbo file */
int
get_xbo_header_length(const char *f_name)
{
  char  buf[MAX_LINE_SIZE]; 
  FILE * f;
  char * token; 
  char * delim =  (char*) " \t,";
  int offset = 0; 
  int not_done = 1;

  f = fopen(f_name, "r"); 
  if (f == NULL) {
    fprintf(stderr, "Could not open input file: %s\nAborting!\n", f_name); 
    exit(-1); 
  }

  while (not_done) {
    fgets(buf, MAX_LINE_SIZE-1, f); 
#ifdef DEBUG
    printf("Line read: [%s]\n", buf); 
#endif
    offset += strlen(buf); 
    
    token = strtok(buf, delim); 
    if (strcmp(token, ".ident") == 0)
      not_done = 0;

  } // while not_done

  int closeRes = fclose(f);
  if (closeRes) {
    printf("Error closing file' %s'\n", f_name);
    return -1;
  }

  return offset; 
}

int 
hexdump_var (const char* f_name, const int* var, long length) {
  FILE * f;

  f = fopen(f_name, "w");
  if (f == NULL) {
    fprintf(stderr, "Could not open output file: %s\nAborting!\n", f_name);
    exit(-1);
  }
  for (long i=0; i<length; i++) {
    fprintf(f, "%08x\n", var[i]);
  }
  int closeRes = fclose(f);
  if (closeRes) {
    printf("Error closing file' %s'\n", f_name);
    return -1;
  }

  return 0;
}
