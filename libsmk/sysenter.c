#include <smk/syscalls.h>

/**  \defgroup SYSENTER  Sysenter hook into the kernel  
 *
 */

/** @{ */


void  _sysenter( struct syscall_packet *scp )
{
  asm (   "  mov $sysentry_return, %%edx;\n"
		  "  mov %%esp, %%ecx;\n"
		  "  mov %0, %%eax;\n"
		  "  sysenter;\n"
	  	  "sysentry_return:\n"
		  : 
		  : "p" (scp) 
		  : "eax", "ecx", "edx"	);
}

/** @} */
