#ifndef _KERNEL_SYSTEM_H
#define _KERNEL_SYSTEM_H

#include <smk/systemdefs.h>



int	get_system_info( struct system_info *info );

int	get_process_info( int pid, struct process_info *info );



#endif

