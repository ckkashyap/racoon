#ifndef _LIBKERNEL_SYSTEMDEFS_H
#define _LIBKERNEL_SYSTEMDEFS_H

#include "limits.h"
#include "inttypes.h"



struct system_info
{
	char	kernel[ SMK_NAME_LENGTH ];
	int		process_count;

};


struct process_info
{
	char	name[ SMK_NAME_LENGTH ];
	int 	pid;
	int		ppid;
	int		thread_count;
};

#endif
