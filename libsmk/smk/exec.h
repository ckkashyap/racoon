#ifndef _LIBKERNEL_EXEC_H
#define _LIBKERNEL_EXEC_H

#ifdef __cplusplus
extern "C" {
#endif

int smk_mem_exec( const char *name, 
				  uintptr_t start, 
				  uintptr_t end, 
				  const char *command_line );


#ifdef __cplusplus
}
#endif

#endif

