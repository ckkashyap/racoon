#include <smk/inttypes.h>
#include <smk/err.h>
#include <smk/syscalls.h>
#include "../include/ualloc.h"
#include <processes/process.h>
#include <scheduler/scheduler.h>



void syscall_mem( struct syscall_packet *scp )
{
	struct process *proc = NULL;
	scp->rc = SMK_UNKNOWN_SYSCALL;

	switch (scp->opcode)
	{
		case OP( MEMORY, ONE ):
				proc = checkout_process( getpid(), WRITER );
				assert( proc != NULL );
				scp->rc = user_alloc( current_process(), scp->size, &(scp->ptr) );
				commit_process( proc );
				break;

		case OP( MEMORY, TWO ):
				proc = checkout_process( getpid(), WRITER );
				assert( proc != NULL );
				scp->rc = user_free( current_process(), scp->ptr );
				commit_process( proc );
				break;

		case OP( MEMORY, THREE ):
				proc = checkout_process( getpid(), READER );
				assert( proc != NULL );
				scp->rc = user_size( current_process(), scp->ptr, &(scp->size) );
				commit_process( proc );
				break;

		case OP( MEMORY, FOUR ):
				proc = checkout_process( getpid(), READER );
				assert( proc != NULL );
				scp->rc = user_location( current_process(), scp->ptr, &(scp->ptr) );
				commit_process( proc );
				break;

		case OP( MEMORY, FIVE ):
				proc = checkout_process( getpid(), WRITER );
				assert( proc != NULL );
				scp->rc = user_map( current_process(), scp->ptr, scp->size, &(scp->ptr) );
				commit_process( proc );
				break;

	}

}

