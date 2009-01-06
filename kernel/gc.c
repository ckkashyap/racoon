#include <smk/process_info.h>
#include <debug/assert.h>
#include "include/misc.h"
#include "include/eflags.h"
#include "include/cpu.h"
#include <scheduler/scheduler.h>
#include <processes/process.h>
#include <ds/rwlock.h>


/** The garbage collector is responsible for the following:
 *
 *    1. Cleaning up of threads
 *    2. Cleaning up of processes
 *
 * It needs to be a separate thread to prevent deadlock
 * situations.
 *
 * These functions disable interrupts to provide atomic operations
 * and can do this because they're run by each scheduler and cpu
 * individually.
 * 
 */

#define		MAX_GC		32


/** This structure contains information about the object
 * requiring death 
 */
struct gc_info
{
	int pid;		///< The pid of the object
	int tid;		///< The tid of the object
};


/** This structure is the queue of objects requiring destruction.  */
static struct
{
	struct gc_info	info[ MAX_GC ]; ///< The objects
	int				pos;			///< The qdata position
	int				size;			///< The num of qdatad objs

} qdata[ MAX_CPU ];



/** Load function on CPU initialization */
void load_gc( int cpu_id )
{
	qdata[ cpu_id ].pos = 0;
	qdata[ cpu_id ].size = 0;
}



/** One shot attempt to queue an object for deletion.
 *
 * If it returns failure, the caller should release to the
 * scheduler and try again on the next timeslice. Repeat
 * until this function works.
 *
 *
 * \note safe for interrupts disabled/enabled mode.
 * 
 * \return 0 if successfully qdatad.
 */
int  gc_queue( int pid, int tid )
{
	int cpu_id = CPUID;
	int ei = disable_interrupts();	
	int rc = -1;

	assert( qdata[ cpu_id ].pos >= 0 );
	assert( qdata[ cpu_id ].size >= 0 );

		if ( qdata[ cpu_id ].size != MAX_GC )
		{
			int new_pos = qdata[ cpu_id ].pos + qdata[ cpu_id ].size;
			    new_pos = new_pos % MAX_GC;

			qdata[ cpu_id ].info[ new_pos ].pid = pid; 
			qdata[ cpu_id ].info[ new_pos ].tid = tid; 

			qdata[ cpu_id ].size += 1;
			rc = 0;
		}


	if ( ei != 0 ) enable_interrupts();
	return rc;
}


/** This function should only be called by the scheduler to
 * determine if the garbage collector needs to run.
 *
 * Run determination is based on the number of objects still
 * in the queue.
 *
 * \return 0 if the gc has work to do
 * \return -1 if there is nothing to do.
 */
int	gc_has_work( int cpu_id )
{
	assert( (cpu_flags() & EFLAG_INTERRUPT) == 0 );

	assert( qdata[ cpu_id ].pos >= 0 );
	assert( qdata[ cpu_id ].size >= 0 );

	if ( qdata[ cpu_id ].size == 0 ) return -1;

	return 0;
}


/** Removes the next pid & tid from the list of things
 * to be processed, if there is any.
 *
 * \note Does not increment the position or decrement the size of the queue.
 *
 * \note safe for interrupts disabled/enabled mode.
 *
 * \return 0 if there is data waiting to be processed.
 */
static int	gc_peek( int cpu_id, int *pid, int *tid )
{
	int ei = disable_interrupts();	
	int rc = -1;

	assert( qdata[ cpu_id ].pos >= 0 );
	assert( qdata[ cpu_id ].size >= 0 );

		if ( qdata[ cpu_id ].size != 0 )
		{
			int new_pos = qdata[ cpu_id ].pos;

			*pid = qdata[ cpu_id ].info[ new_pos ].pid; 
			*tid = qdata[ cpu_id ].info[ new_pos ].tid; 

			rc = 0;
		}


	if ( ei != 0 ) enable_interrupts();
	return rc;
}


/** Steps over to the next entry in the queue 
 *
 * \note safe for interrupts disabled/enabled mode.
 *
 */
static void gc_next( int cpu_id )
{
	int ei = disable_interrupts();	

		assert( qdata[ cpu_id ].pos >= 0 );
		assert( qdata[ cpu_id ].size > 0 );
	
		qdata[ cpu_id ].pos = (qdata[ cpu_id ].pos + 1) % MAX_GC;
		qdata[ cpu_id ].size -= 1;

	if ( ei != 0 ) enable_interrupts();
}



static int gc_do_thread( int cpu_id, int pid, int tid )
{
	struct process *proc;
	struct thread *tr;
	int rc = 0;


	proc = checkout_process( pid, WRITER );
	if ( proc == NULL ) return -1;

		tr = get_thread( proc, tid );
		if ( tr == NULL ) return -1;

		assert( tr->state == THREAD_FINISHED );
		assert( tr->state != THREAD_IRQ );

		// Delete it!
		rc = delete_thread( tr );
		assert( rc == 0 );

	commit_process( proc );
	return 0;
}

/** Deletes a process */
static int gc_do_process( int cpu_id, int pid )
{
	struct process *proc;
	int remaining = 1;
	int rc;

	// Try get the process. Wait for all threads to exit the kernel.
	assert( remaining != 0 );	// Tsk. Set it back.
	while ( remaining != 0 )
	{
		proc = checkout_process( pid, WRITER );
		if ( proc == NULL ) return -1;
		assert( proc->state == PROCESS_FINISHING );
		remaining = proc->kernel_threads;
		if ( remaining != 0 ) 
		{
#warning work around for my less than good scheduler design
			struct thread *tr = proc->threads;
			while ( tr != NULL )
			{
				assert( tr->state != THREAD_IRQ );

				if ( tr->state == THREAD_DORMANT )
						set_thread_state( tr, THREAD_RUNNING );
				tr = NEXT_LINEAR( tr, struct thread* );
			}
					
			commit_process( proc );
		}
		
	}
	assert( proc != NULL );

	// The process is mine!	Delete the threads.
	while ( proc->threads != NULL )
	{
		set_thread_state( proc->threads, THREAD_FINISHED );

		rc = delete_thread( proc->threads );
		assert( rc == 0 );
	}
	assert( proc->threads == NULL );

	
	// All the threads should be deleted now.
	release_processes();
	remove_process( proc );

#warning DELETING A PROCESS BUT SOMEONE ELSE COULD BE ACQUIRING IT
	delete_process( proc );	

	return 0;
}


/** This is the gc thread. Should only be run by the scheduler
 * when there's work to do. */
void gc()
{
	int cpu_id = CPUID;

	while (1==1) 
	{ 
		int pid;
		int tid;

		assert( (cpu_flags() & EFLAG_INTERRUPT) != 0 );

		if ( gc_peek( cpu_id, &pid, &tid ) != 0 )
		{
			sched_yield();			// Release to scheduler
			continue;
		}
		
		// Now we have a pid and a tid to destroy
			if ( tid < 0 )	
				gc_do_process( cpu_id, pid );  // Process destruction
			else			
				gc_do_thread( cpu_id, pid, tid ); // Thread destruction

		gc_next( cpu_id );
	}
}












