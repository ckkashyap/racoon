#ifndef _KERNEL_EXEC_H
#define _KERNEL_EXEC_H

#include <smk/inttypes.h>

int exec_memory_region( int pid,
	    				char *name, 
                   		uintptr_t start, 
	    				uintptr_t end, 
	    				const char *parameters );


#endif

