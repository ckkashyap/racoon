#ifndef _LIBKERNEL_STRINGS_H
#define _LIBKERNEL_STRINGS_H

#include "inttypes.h"


extern char *process_state[];
extern char *thread_state[];
extern char *exception_strings[];


void* 	kmemcpy(void* s, const void* ct, size_t n);
int 	kmemcmp(const void* s, const void* d, size_t n);
void* 	kmemset(void* s, int c, size_t n); 
char* 	kstrcpy(char* s, const char* ct);
char* 	kstrncpy(char* s, const char* ct, size_t n);
size_t 	kstrlen(const char* cs);
int		kstrncmp(const char *s1, const char *s2, size_t n);



void 	kzero_bss();

#endif

