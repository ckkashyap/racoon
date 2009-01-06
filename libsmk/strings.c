#include <smk/inttypes.h>

/**  \defgroup STRINGS  Strings  
 *
 */

/** @{ */



char *process_state[] = 
{
    "system",
    "running",
    "stopped",
    "finishing",
    "finished",
    "crashing",
    "crashed"
};

char *thread_state[] =
{
	"system",
	"running",
	"suspended",
	"dormant",
	"stopped",
	"semaphore",
	"sleeping",
	"waiting",
	"finished",
	"crashed",
	"irq"
};


char *exception_strings[] = 
{
    "Divide-by-zero",			//	0
    "Debug Exception",			//	1
    "NMI",				//	2
    "Breakpoint",			//	3
    "INTO",				//	4
    "BOUNDS",				//	5
    "Invalid Opcode",			//	6
    "Device Not Available",		//	7
    "Double-fault",			//	8
    "Coprocessor segment overrun",	//	9
    "Invalid TSS fault",		//	10
    "Segment Not Present",		//	11
    "Stack Exception",			//	12
    "General Protection",		//	13
    "Page Fault",			//	14
    "*reserved*",			//	15
    "Floating-point error",		//	16
    "Alignment Check",			//	17
    "Machine Check",			//	18
    "*reserved*", 			//	19
    "*reserved*", 			//	20
    "*reserved*", 			//	21
    "*reserved*", 			//	22
    "*reserved*", 			//	23
    "*reserved*", 			//	24
    "*reserved*", 			//	25
    "*reserved*", 			//	26
    "*reserved*", 			//	27
    "*reserved*", 			//	28
    "*reserved*", 			//	29
    "*reserved*", 			//	30
    "*reserved*" 			//	31
};           


void* 	kmemcpy(void* s, const void* ct, size_t n)
{
	char *dest,*src;
	size_t i;

	dest = (char*)s;
	src = (char*)ct;
	for ( i = 0; i < n; i++)
		dest[i] = src[i];
	
	return s;
}

int 	kmemcmp(const void* s, const void* d, size_t n)
{
	size_t i;

	for ( i = 0; i < n; i++ )
	{
		if ( ((const char*)s)[i] <  ((const char*)d)[i] ) return -1;
		if ( ((const char*)s)[i] >  ((const char*)d)[i] ) return 1;
	}

	return 0;
}


void* 	kmemset(void* s, int c, size_t n)
{
	size_t i;
	for ( i = 0; i < n ; i++)
		((char*)s)[i] = c;
	
	return s;
}


char* 	kstrcpy(char* s, const char* ct)
{
	size_t i = 0;
	do { s[i] = ct[i]; } 
		while (ct[i++] != '\0');
	return s;
}

char* 	kstrncpy(char* s, const char* ct, size_t n)
{
	size_t i;

	for (i = 0 ; i < n && ct[i] != '\0' ; i++)
		s[i] = ct[i];
	for ( ; i < n ; i++)
		s[i] = '\0';

	return s;
}

 
size_t 	kstrlen(const char* cs)
{
	size_t ans = 0;
	while ( cs[ans++] != '\0' ) {}
	return (ans-1);
}


int kstrncmp(const char *s1, const char *s2, size_t n)
{
	size_t i = 0;

	while ( i < n )
	{
		if ( s1[i] < s2[i] ) return -1;
		if ( s2[i] < s1[i] ) return +1;

		if ( s1[i] == 0 ) break;

		i += 1;
	}

	return 0;
}



extern char __bss_start;
extern char __bss_end;

void kzero_bss()
{
  char *bss = &__bss_start;
  while ( bss != &__bss_end ) *bss++ = 0;
}



/** @} */

