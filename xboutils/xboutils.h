#ifndef __XBOUTILS_H__
#define __XBOUTILS_H__
/* Utility functions for reading from XBO files */

int 
read_int_data_var(const char * f_name, const void * dest, const char* v_name);

extern int 
read_int_data(const char * f_name, const void * dest, const int f_offset, const int length);

extern int 
get_var_length_xbo(const char * f_name, const char * var_name);

extern int 
get_var_offset_xbo(const char * f_name, const char * var_name);

extern int
get_xbo_header_length(const char *f_name);

int
hexdump_var(const char* f_name, const int* var, long length);

#endif
