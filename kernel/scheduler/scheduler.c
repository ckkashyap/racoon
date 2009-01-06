#include <smk/inttypes.h>
#include <smk/atomic.h>
#include "include/interrupts.h"
#include "include/cpu.h"
#include <debug/assert.h>
#include <mm/mm.h>

#include "include/eflags.h"
#include "include/misc.h"



#include "include/smk.h"
#include "include/gc.h"

#include <processes/threads.h>


#include "include/apic.h"

#include <ds/rwlock.h>

#include "include/stats.h"

#define INITIAL_SCHED_SIZE		1024
#define TIMESLICE				10
							// in milliseconds



void load_scheduler( unsigned int cpu_id )
{
	struct scheduler_info *si = &(cpu[ cpu_id ].sched);

	int i = sizeof( struct schedule ) * INITIAL_SCHED_SIZE;
	si->sched_queue = (struct schedule*)kmalloc( i );

	i = sizeof( struct paused_schedule ) * INITIAL_SCHED_SIZE;
	si->pause_queue = (struct paused_schedule*)kmalloc( i );

	for ( i = 0; i < INITIAL_SCHED_SIZE; i++ )
	{
		si->sched_queue[i].tr = NULL;
		si->pause_queue[i].tr = NULL;
	}


	si->cpu_id = cpu_id;

	si->current_thread = NULL;

	si->requesting_state = -1;
	si->requesting_thread = NULL;
	
	si->sched_count = 0;
	si->pause_count = 0;
	si->sched_size = INITIAL_SCHED_SIZE;
	si->pause_size = INITIAL_SCHED_SIZE;

	si->lock_scheduler = INIT_SPINLOCK;
	si->nested_lock = 0;
	
	si->running = 0;
	si->idle = 0;

	si->position = 0;

	si->fast_start = 0;
	si->fast_stop = 0;
}





static int sub_queue( struct scheduler_info *si, struct thread *tr )
{
	sched_lock( si->cpu_id, 0 );

		if ( si->sched_count == si->sched_size ) 
		{
			// too damn full, damn it.	
			sched_unlock( si->cpu_id );
			return -1;
		}


		// Add it.
		
		si->sched_queue[ si->sched_count ].tr 	    = tr;
		si->sched_count += 1;
	
	sched_unlock( si->cpu_id );
	return 0;
}


static int sub_dequeue( struct scheduler_info *si, struct thread *tr )
{
	int pos;
	int i;

	// You can not dequeue yourself!
	assert( tr != current_thread() );
	
	if ( si->cpu_id == CPUID )
		sched_lock( si->cpu_id, 0 );
	else
		sched_lock( si->cpu_id, 1 );

	
		// Find it's position.
		for ( pos = 0; pos < si->sched_count; pos++ )
			if ( si->sched_queue[ pos ].tr == tr ) break;

		assert( pos != si->sched_count );	// It was actually there.

		// Close the gap. (I know it's not ideal)
		for ( i = pos; i < (si->sched_count - 1); i++ )
			si->sched_queue[ i ] = si->sched_queue[ i + 1 ];

		// Clean & keep track
		si->sched_count -= 1;

		if ( si->sched_count == 0 ) si->position = 0;
		else
			si->position = ((si->position-1) % si->sched_count);
							// safe because position is unsigned


	sched_unlock( si->cpu_id );
	return 0;
}


int pause( struct thread *tr, uint64_t milliseconds )
{
	int pos, i, id;
	struct scheduler_info *si;
	uint64_t now;
	uint64_t target;


	assert( tr != NULL );			// Real thread.
	assert( milliseconds > 0 );		// Real time

	
	assert( tr->cpu == CPUID );		// Current CPU

	id = tr->cpu;
	si = &( cpu[ id ].sched );
	
	sched_lock( id, 0 );

		now = cpu[ id ].st_systemTime.usage;
		target = milliseconds + now;
	
	// PAUSE INSERTION ......................
		
		if ( si->pause_count == si->pause_size ) 
		{
			sched_unlock( id );
			return -1;
		}

		// Add it by finding the position in the queue.
		
		for ( pos = 0; pos < si->pause_count; pos++ )
		{
			if ( si->pause_queue[ pos ].request > target ) break; 
		}
		
		// Move everything up one.
		for ( i = si->pause_count; i > pos; i-- )
		{
			si->pause_queue[ i ] = si->pause_queue[ i - 1 ];
		}

		// Insert the pause
		
		si->pause_queue[ pos ].tr 	    = tr;
		si->pause_queue[ pos ].state 	= tr->state;
		si->pause_queue[ pos ].request 	= target;
		si->pause_queue[ pos ].start 	= now;
		si->pause_count += 1;

		set_thread_state( tr, THREAD_DORMANT );
		tr->duration = milliseconds; 

	sched_unlock( id );
	return 0;
}




/** Actually moves the thread to position 0 on the pause queue and waits
 * for the scheduler to remove it */
int restart( struct thread *tr )
{
	int pos, i, id;
	struct scheduler_info *si;
	struct paused_schedule ps;

	assert( tr != NULL );			// Real thread.

	id = tr->cpu_affinity;
	if ( id < 0 ) id = ((tr->tid - 1) % cpu_count);
	
	si = &( cpu[ id ].sched );
	

	if ( id == CPUID )
		sched_lock( id, 0 );
	else
		sched_lock( id, 1 );

	// PAUSE INSERTION ......................
		
		// Find the position in the queue
		
		for ( pos = 0; pos < si->pause_count; pos++ )
		{
			if ( si->pause_queue[ pos ].tr == tr ) break; 
		}

		// Was it there?
		if ( pos == si->pause_count )
		{
			sched_unlock( id );
			assert( "unpausing a non-paused thread" == "true" );
			return -1;
		}

		if ( pos == 0 )
		{
			si->pause_queue[ pos ].request = 0;
			sched_unlock( id );
			return 0;
		}

		ps = si->pause_queue[ pos ];
		
		// Move everything down one.
		for ( i = pos; i > 0; i-- )
		{
			si->pause_queue[ i ] = si->pause_queue[ i - 1 ];
		}

		si->pause_queue[ 0 ] = ps;
		si->pause_queue[ 0 ].request = 0;

	sched_unlock( id );
	return 0;
}


// ****

/// \todo Improve allocation techniques..

int queue( struct thread *tr )
{
	int id = tr->cpu_affinity;
	if ( id < 0 ) id = ((tr->tid - 1) % cpu_count);

	assert( tr != NULL );
	assert( tr->cpu < 0 );

	tr->cpu = id;

	return sub_queue( &( cpu[ id ].sched ), tr );
}

int dequeue( struct thread *tr )
{
	int ans;
	int id = tr->cpu;

	assert( tr != NULL );
	assert( tr->cpu >= 0 );
	
	ans = sub_dequeue( &( cpu[ id ].sched ), tr );
	tr->cpu = -1;

	return ans;
}





void remove_last( struct scheduler_info *si )
{
	int pos,i;
	// Honour the thread's requested state change
	
	if ( (si->requesting_thread != NULL)  && (si->requesting_state >= 0))
	{
		struct thread *tr = si->requesting_thread;
				
		// Find it's position.
		for ( pos = 0; pos < si->sched_count; pos++ )
			if ( si->sched_queue[ pos ].tr == tr ) break;
		
		// Was it actually here?
		if ( pos == si->sched_count ) 
		{
			si->requesting_thread = NULL;
			si->requesting_state = -1;
			return;
		}

		// Close the gap. (I know it's not ideal)
		for ( i = pos; i < (si->sched_count - 1); i++ )
			si->sched_queue[ i ] = si->sched_queue[ i + 1 ];

		// Clean & keep track
		si->sched_count -= 1;

		if ( si->sched_count == 0 ) si->position = 0;
		else
			si->position = ((si->position-1) % si->sched_count);
							// safe because position is unsigned

		tr->state = si->requesting_state;
		tr->cpu = -1;

		/// \todo  This is where I would hook into the sleep/dormancy tables

		si->requesting_thread = NULL;
		si->requesting_state = -1;
	}

	assert( si->requesting_thread == NULL );
	assert( si->requesting_state < 0 );
}


/* Ensure that timed sleepers are woken up on schedule and
 * their time reported.
 */
uint64_t remove_timers( struct scheduler_info *si, uint64_t time )
{
	struct thread *tr;
	int count = 0;
	int i;

	if ( si->pause_count == 0 ) return 0;

	while ( si->pause_queue[count].request <= time )
	{
		tr = si->pause_queue[count].tr;
		tr->state = si->pause_queue[count].state;
		tr->duration = time - si->pause_queue[count].start;
		tr->cpu = si->cpu_id;
		
		assert( si->sched_count != si->sched_size );

		// Add it back in.
		si->sched_queue[ si->sched_count ].tr = tr;
		si->sched_count += 1;

		count += 1;

		si->pause_count -= 1;
		
		if ( si->pause_count == 0 ) break;
	}

	
	for ( i = 0; i < si->pause_count; i++ )
	{
		si->pause_queue[i] = si->pause_queue[i+count];  
	}

	if ( si->pause_count == 0 ) return 0;

	return ( si->pause_queue[0].request - time );
}



int fast_queue( struct thread *tr )
{
	unsigned int new_stop;
	
	int id = tr->cpu_affinity;
	assert( (id>=0) && (id<cpu_count) );

	sched_lock( id, 0 );

		tr->cpu = id;
		tr->fast_queued += 1;
	
		new_stop = cpu[ id ].sched.fast_stop;

		cpu[ id ].sched.fast_queue[ new_stop ] = tr;
		cpu[ id ].sched.fast_stop = (new_stop + 1) % MAX_FAST;

		assert( cpu[ id ].sched.fast_stop != cpu[ id ].sched.fast_start );

	sched_unlock( id );
	return 0;
}

int fast_dequeue( struct thread *tr )
{
	int id = tr->cpu;

	assert( id >= 0 );

	sched_lock( id, 0 );
	assert( tr->cpu == CPUID );
	assert( tr == cpu[ id ].sched.fast_queue[ cpu[ id ].sched.fast_start ] );

		cpu[ id ].sched.fast_start = (cpu[ id ].sched.fast_start + 1) % MAX_FAST;
		tr->fast_queued -= 1;

		if ( tr->fast_queued == 0 )
			tr->cpu = -1;

	sched_unlock( id );
	return 0;
}	


/* Basically, this function is guaranteed to return
 * when the target CPU/scheduler is locked for you to
 * use.
 */
void sched_lock( int cpu_id, int complete )
{
	int lock = 0;
	
	assert( (cpu_id >= 0) && (cpu_id < cpu_count) );
	assert( ! ((CPUID == cpu_id) && (complete == 1 )) );	
				// Because a complete lock requires the scheduler to halt
				// the running thread. But if the thread is running on the
				// same CPU and it's halted, then we're in trouble.

	//if ( CPUID == cpu_id ) lock = disable_interrupts();
	lock = disable_interrupts();
	
	acquire_spinlock( &(cpu[ cpu_id ].sched.lock_scheduler) );
	cpu[ cpu_id ].sched.lock_flags = lock;
	

	// If it's idle, wake it up..
	if ( cpu[ cpu_id ].sched.idle != 0 )
	{
		// Mark it as not-idle and wake it up.
		cpu[ cpu_id ].sched.idle = 0;
		send_interrupt( cpu_id, APIC_INTERRUPT );
	}
	

	if ( complete == 1 )
	{
		while ( cpu[ cpu_id ].sched.running != 0 );
	}


	assert( cpu[ cpu_id ].sched.nested_lock++ == 0 );
}

/** Same as sched_lock except in reverse.
 */
void sched_unlock( int cpu_id )
{
	int enable = cpu[ cpu_id ].sched.lock_flags;
		 
	assert( (cpu_id >= 0) && (cpu_id < cpu_count) );
	assert( cpu[ cpu_id ].sched.nested_lock-- == 1 );
		
	release_spinlock( &(cpu[ cpu_id ].sched.lock_scheduler) );

	if ( CPUID == cpu_id ) 
		if ( enable != 0 ) enable_interrupts();
}


/** This is the global scheduler lock. Only 1 CPU may 
 * attempt to lock every other scheduler at any 1 time. 
 */
static spinlock_t sched_global_lock = INIT_SPINLOCK;

/** This should be called when you want every single CPU to
 * be locked. This is useful for the IRQ handler modifications.
 * In fact, that is currently the only place that it is used.
 *
 * It's important that you limit the work you do during this
 * giant lock to very simple stuff. Particularly value swapping.
 * No memory allocations or anything!
 *
 * Also, the other CPU's will be in a non-interruptable state.
 */
void sched_lock_all()
{
	int i;
	int cpu_id = CPUID;

	acquire_spinlock( & sched_global_lock );

	for ( i = 0; i < cpu_count; i++ )
	{
		if ( cpu_id == i )
			sched_lock( i, 0 );
		else
			sched_lock( i, 1 );
		
	}

}

/** Unlocks all the CPU's after a sched_lock_all call. */
void sched_unlock_all()
{
	int i;
	for ( i = 0; i < cpu_count; i++ )
	{
		sched_unlock( i );
	}
	release_spinlock( & sched_global_lock );

}




/** Ensure that this is only ever called from within the scheduler */
struct thread *get_fast( struct scheduler_info *si )
{
	if ( si->fast_start == si->fast_stop ) return NULL;
	return si->fast_queue[ si->fast_start ];
}


void scheduler()
{
	unsigned int cpu_id = CPUID; 
	struct scheduler_info *si = &(cpu[ cpu_id ].sched);
	struct thread *tr;
	int idle;
	int flip = 0;
	uint64_t requested_time = TIMESLICE;

	// We will never go back.
	set_cpu_flags(  cpu_flags() & ~EFLAG_NESTED_TASK );

	// Now we're working.
	acquire_spinlock( &(si->lock_scheduler) );

	// Start timing the scheduler
	stats_time_start( &(cpu[ cpu_id ].st_schedulerTime) );

	assert( (cpu_flags() & EFLAG_INTERRUPT) == 0 );

	ack_apic();

	while (1==1)
	{
		// Just show the world that we're still alive.
		((char*)0xB8000)[158 - cpu_id * 2] ++ ;		/// \todo remove one day


		// If the garbage collector has work to do, let it run.
		if ( gc_has_work( cpu_id ) == 0 )
		{
			tr = smk_gc[ cpu_id ];
			exec_thread( cpu_id, tr, TIMESLICE, 0 );
			stats_time( cpu_id, &(cpu[ cpu_id ].st_systemTime) ); 
		}

		
		// Find out when the next timed event is.
		requested_time = remove_timers( si, cpu[ cpu_id ].st_systemTime.usage );
		idle = 0;
			
		// Fast queue support
		if ( flip == 0 )
		{
			tr = get_fast( si );
			if ( tr != NULL )
			{
				exec_thread( cpu_id, tr, TIMESLICE, 0 );
				stats_time( cpu_id, &(cpu[ cpu_id ].st_systemTime) ); 
														// Maintain CPU time.

				if ( si->sched_count != 0 ) flip = 1;	// Ensure others run
				continue;
			}
		}


		flip = 0;
		// Reset of fast queue
		
		// If there's nothing to do, do idle.
		if ( si->sched_count == 0 ) 
		{
			if ( gc_has_work( cpu_id ) == 0 ) continue;	// Al-e-oop!
				
			tr = smk_idle[ cpu_id ];
			idle = 1;		// Safe to wake up when required.
		}
		else
		{
			tr = si->sched_queue[ si->position ].tr;
			si->position = (si->position + 1) % si->sched_count;

			requested_time = TIMESLICE;
			idle = 0;		// Don't interrupt until timeslice is over
		}


		// And run the selected thread.
		exec_thread( cpu_id, tr, requested_time, idle );
		stats_time( cpu_id, &(cpu[ cpu_id ].st_systemTime) ); 
														// Maintain CPU time.
														//
		// If the previous thread requested a state change, honour it.
		remove_last( si );

	}

}


