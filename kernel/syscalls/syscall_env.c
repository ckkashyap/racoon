#include <smk/inttypes.h>
#include <smk/err.h>
#include <smk/syscalls.h>
#include "../include/env.h"
#include <processes/process.h>
#include <scheduler/scheduler.h>



void syscall_env( struct syscall_packet *scp )
{
	struct process *proc = NULL;
	scp->rc = SMK_UNKNOWN_SYSCALL;

	switch (scp->opcode)
	{
		case OP( ENV, ONE ):
				proc = checkout_process( getpid(), WRITER );
				assert( proc != NULL );
				scp->rc = set_environment( current_process(), scp->name, scp->ptr, scp->size );
				commit_process( proc );
				break;

		case OP( ENV, TWO ):
				proc = checkout_process( getpid(), READER );
				assert( proc != NULL );
				scp->rc = get_environment( current_process(), scp->name, scp->ptr, &(scp->size) );
				commit_process( proc );
				break;

		case OP( ENV, THREE ):
				proc = checkout_process( getpid(), READER );
				assert( proc != NULL );
				scp->rc = get_environment_size( current_process(), scp->name, &(scp->size) );
				commit_process( proc );
				break;

		case OP( ENV, FOUR ):
				proc = checkout_process( getpid(), READER );
				assert( proc != NULL );
				scp->rc = get_environment_information( current_process(), scp->id, scp->name, &(scp->size) );
				commit_process( proc );
				break;

		case OP( ENV, FIVE ):
				proc = checkout_process( getpid(), WRITER );
				assert( proc != NULL );
				scp->rc = remove_environment( current_process(), scp->name );
				commit_process( proc );
				break;

	}

}

