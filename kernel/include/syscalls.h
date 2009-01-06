#ifndef _KERNEL_SYSCALLS_H
#define _KERNEL_SYSCALLS_H




void __system_call();

void syscall_prepare( uint32_t edi, uint32_t esi, uint32_t ebp,
					 uint32_t temp, uint32_t ebx, uint32_t edx,
					 uint32_t ecx, uint32_t eax );

#include <smk/syscalldefs.h>




#endif


