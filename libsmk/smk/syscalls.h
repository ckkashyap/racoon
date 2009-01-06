#ifndef _LIBKERNEL_SYSCALLS_H
#define _LIBKERNEL_SYSCALLS_H



/**  \defgroup SYSCALLS  System Call Defines and Structure
 *
 */

/** @{ */

#include <smk/inttypes.h>
#include <smk/limits.h>
#include <smk/syscalldefs.h>


/** Passed during a system call to the spoon microkernel.  */
struct	syscall_packet
{
	unsigned int	opcode;

	int 	pid;
	int 	tid;
	int 	id;

	int		dest_port;
	int		src_port;

	size_t	size;
	void*	ptr;

	char	name[SMK_NAME_LENGTH];

	unsigned int	flags;
	unsigned int	state;

	unsigned long	data[6];

	int		rc;
};

/** @} */



#endif

