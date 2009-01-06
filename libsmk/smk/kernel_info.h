#ifndef _LIBKERNEL_KERNEL_INFO_H
#define _LIBKERNEL_KERNEL_INFO_H

#include <smk/inttypes.h>
#include <smk/limits.h>


struct process_information
{
	char 			name[SMK_NAME_LENGTH];
	char			cmdline[SMK_CMDLINE_LENGTH];
	int 			pid;
	unsigned int	state;	
};



#endif

