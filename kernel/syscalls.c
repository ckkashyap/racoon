#include <smk/inttypes.h>
#include <smk/err.h>
#include <smk/syscalls.h>
#include <smk/process_info.h>


#include <processes/process.h>
#include <processes/threads.h>

#include <scheduler/scheduler.h>
#include "include/interrupts.h"


extern void syscall_mem( struct syscall_packet *scp );
extern void syscall_env( struct syscall_packet *scp );
extern void syscall_system( struct syscall_packet *scp );




void syscall( struct syscall_packet *scp )
{
	// Dying..... stop all threads from entering the kernel.
	if ( (current_process()->state == PROCESS_CRASHING) || 
		 (current_process()->state == PROCESS_FINISHING)    )
	{
		while (1==1) sched_yield();
	}


	atomic_inc( & (current_process()->kernel_threads) );

	scp->rc = SMK_UNKNOWN_SYSCALL;
	  
	switch ( SYSCALL_OP( scp->opcode) )
	{
		case SYS_MEMORY: syscall_mem( scp ); break;
		case SYS_ENV: syscall_env( scp ); break;
		case SYS_SYSTEM: syscall_system( scp ); break;
	}
  
	atomic_dec( & (current_process()->kernel_threads) );
}



/** Returns 0 if syscall is safe while interrupts are disabled. */
int syscall_intsafe( unsigned int opcode )
{
	if ( opcode == OP( IO, TWO ) ) return 0;
	if ( opcode == OP( TIME, TWO ) ) return 0;
	return -1;
}



void syscall_prepare( struct syscall_packet *scp )
{
	if ( current_thread()->interrupts_disabled == 0 ) enable_interrupts();
	else
	{
		if ( syscall_intsafe( scp->opcode ) != 0 )
		{
			scp->rc = SMK_UNSAFE_SYSCALL_INT;
			return;
		}
	}

	syscall( scp );
}


