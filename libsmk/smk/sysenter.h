#ifndef _LIBKERNEL_SYSENTER_H
#define _LIBKERNEL_SYSENTER_H

#include <smk/syscalls.h>

void  _sysenter( struct syscall_packet *scp );


#endif


