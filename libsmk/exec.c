#include <smk.h>




/**  \defgroup EXEC  Process Execution  
 *
 */

/** @{ */

int smk_mem_exec( const char *name, 
				 	uintptr_t start, 
		 			uintptr_t end,
		 			const char *command_line)
{
	return _sysenter( (SYS_EXEC|SYS_ONE), 
						(uint32_t)name, 
						(uint32_t)start,
						(uint32_t)end, 
						(uint32_t)command_line,
						0);
}

/** @} */

