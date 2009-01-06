#include <smk/systemdefs.h>
#include <smk/err.h>
#include <smk/syscalls.h>
#include <smk/systemdefs.h>
#include "include/system.h"



void syscall_system( struct syscall_packet *scp )
{
	scp->rc = SMK_UNKNOWN_SYSCALL;

	switch (scp->opcode)
	{
		case OP( SYSTEM, ONE ):
				scp->rc = get_system_info( scp->ptr );
				break;

		case OP( SYSTEM, TWO ):
				scp->rc = get_process_info( scp->pid, scp->ptr );
				break;

	}

}

