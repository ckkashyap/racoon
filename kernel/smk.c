#include <processes/process.h>
#include <debug/assert.h>
#include <processes/threads.h>
#include "include/eflags.h"
#include "include/misc.h"

#include "include/env.h"

#include "include/cpu.h"
#include "include/time.h"

#include "include/gc.h"

struct process *smk_process = NULL;
struct thread  *smk_idle[MAX_CPU];
struct thread  *smk_gc[MAX_CPU];


void idle()
{
	while (1==1) 
	{ 
		assert( (cpu_flags() & EFLAG_INTERRUPT) != 0 );
		asm ("pause; hlt;"); 
	}
}






void init_smk()
{
	int i;
		
	smk_process = new_process( "smk" );

	assert( smk_process != NULL );

	i = set_environment( smk_process, "CMD_LINE", "smk", 4 );
	assert( i >= 0 );

	insert_process( 0, smk_process );
	
	for ( i = 0; i < MAX_CPU; i++ )
	{
		smk_idle[i] = NULL;
		smk_gc[i] = NULL;
	}

}



/** This function spawns the following for each CPU which calls it:
 *
 *
 * 		1.	Idle
 * 		2.  garbage collector
 *
 */
void load_smk( int cpu_id )
{
	int tid;

	struct process* proc = checkout_process( smk_process->pid, WRITER );
	assert( proc != NULL );


	// Idle thread 
		tid = new_thread( 1, proc, NULL, "smk_idle", 
							(uint32_t)idle, 0, 1,2,3,4 );
	
		assert( tid >= 0 );
		smk_idle[ cpu_id ] = get_thread( proc, tid );
		assert( smk_idle[ cpu_id ] != NULL );

		
	// Garbage Collector
		tid = new_thread( 1, proc, NULL, "smk_gc", (uint32_t)gc, 0, 1,2,3,4 );
	
		assert( tid >= 0 );
		smk_gc[ cpu_id ] = get_thread( proc, tid );
		assert( smk_gc[ cpu_id ] != NULL );



	commit_process( proc );
}





