#include <smk/inttypes.h>
#include <smk/atomic.h>
#include <smk/strings.h>

#include <mm/mm.h>

#include "include/eflags.h"

#include <processes/process.h>
#include <processes/threads.h>

#include "include/misc.h"
#include "include/fpu.h"

#include "include/paging.h"
#include "include/ualloc.h"

#include "mm/physmem.h"

#include "include/sysenter.h"

#include "include/stats.h"
#include <ipc/pulse.h>
#include <ipc/messages.h>
#include "include/time.h"

#include "include/apic.h"
#include <ipc/ports.h>

// -----------------------------------------------
//  htable key and compare functions

int threads_key( void *td )
{
	struct thread *t = (struct thread*)td;
	int ans = t->tid * 100;
	if ( ans < 0 ) ans = -ans;
	return ans;
}

int threads_compare( void *ad, void *bd )
{
	struct thread *a = (struct thread*)ad;
	struct thread *b = (struct thread*)bd;

	if ( (b->tid != -1) )
	{
		if ( a->tid < b->tid ) return -1;
		if ( a->tid > b->tid ) return 1;
		return 0;
	}
	
	return kstrncmp( a->name, b->name, SMK_NAME_LENGTH );
}


// -----------------------------------------------

#include <scheduler/scheduler.h>

void set_thread_state( struct thread *t, unsigned int state )
{
	int ei = 0;
	if ( t->state == state ) return;

	// We can't change the state of ourselves so we have to request the
	// scheduler to do it for us.
	if ( t == current_thread() )
	{
		assert( t->state == THREAD_RUNNING );
		ei = disable_interrupts();

			cpu[ CPUID ].sched.requesting_thread = t;
			cpu[ CPUID ].sched.requesting_state = state;

		if ( ei != 0 ) enable_interrupts();
		return;
	}

	// A dormant thread was woken up. Provide the duration.
	if ( (t->state == THREAD_DORMANT) && ( t->duration != 0 ) )
	{
		assert( state == THREAD_RUNNING );
		restart( t );
		return;
	}
	
	if ( t->state == THREAD_RUNNING ) dequeue( t );
	t->state = state;
	if (    state == THREAD_RUNNING ) queue( t );
}


// -----------------------------------------------

int next_tid( struct process *proc )
{
	int tid;
	struct thread temp;
				   temp.name[0] = 0;
				   temp.tid = -1;

		
		do
		{
			if ( temp.tid != -1 )
			{
				proc->last_tid += 1;
				if ( proc->last_tid < 0 ) proc->last_tid = 1;
			}
	
			temp.tid = proc->last_tid;
		}
		while ( htable_retrieve( proc->threads_htable, &temp ) != NULL );

		tid = proc->last_tid++;

	return tid;
}


// -----------------------------------------------


struct thread* get_thread( struct process *proc, int tid )
{
	struct thread temp;
				   temp.name[0] = 0;
				   temp.tid = tid;

		
	return (struct thread*)htable_retrieve( proc->threads_htable, &temp );
}


static inline void pushl( struct thread *tr, uint32_t value )
{
	uint32_t *mem;
	tr->stack = tr->stack - 4;
	mem = (uint32_t*)(tr->stack);
	mem[0] = value;
}





int new_thread( int type,
	       	    struct process *proc, 
		   	    struct thread *parent, 
		        const char name[SMK_NAME_LENGTH],
			    uint32_t entry,
			    int priority,
			    uint32_t data1,
			    uint32_t data2,
			    uint32_t data3,
			    uint32_t data4 )
{
	int i;
		
	struct thread *tr = (struct thread*)kmalloc( sizeof(struct thread) );
	if ( tr == NULL ) return -1;

	kstrncpy( tr->name, name, SMK_NAME_LENGTH );

	tr->tid 	= next_tid( proc );
	tr->state 	= THREAD_SUSPENDED;
	tr->process = proc;


	tr->irq = -1;
	tr->duration = 0;

	for ( i = 0; i < MAX_PORTS; i++ ) 
			tr->ports[i] = -1;
	
		tr->last_port_pulse = -1;
		tr->last_port_message = -1;
	
	

	// architecture ...
	// 

		tr->stack_user = page_find_free( proc->map, 
										    sK_BASE,
											sK_CEILING,
											sK_PAGES_THREAD + 1 );

		tr->stack_user += (sK_PAGES_THREAD + 1) * PAGE_SIZE;

		// We're leaving a guard page at the bottom (and,
		// therefore, the top) of every thread stack.

			user_ensure( proc, 
						 (void*)(tr->stack_user - (sK_PAGES_THREAD * PAGE_SIZE)),
						 sK_PAGES_THREAD );


		tr->stack_kernel = (uint32_t) memory_alloc( sK_PAGES_KERNEL );
		tr->stack_kernel += sK_PAGES_KERNEL * PAGE_SIZE;
	     

	tr->cpu 			= -1;
	tr->cpu_affinity 	= -1;		// No preference.
	tr->fast_queued 	= 0;		// Atomically protected by fast_queue schedlocking
	tr->interrupts_disabled = 0;	// Interrupts are enabled

	tr->rc = 0;						// Return code is 0.

	tr->tls = memory_alloc( 1 );	// TLS memory section
		
	tr->math_state = -1;
	tr->math = memory_alloc( 1 );

	tr->event_tid	= -1;

	tr->stack = tr->stack_kernel;

	// ------------
   
	if ( type == 1 )
	{
		// kernel thread
		pushl( tr, GDT_SYSTEMDATA );
		pushl( tr, tr->stack_user );
		pushl( tr, EFLAG_BASE | EFLAG_INTERRUPT  );
		pushl( tr, GDT_SYSTEMCODE );
	}
	else
	{
		// user thread
		pushl( tr, GDT_USERDATA | 3 );
		pushl( tr, tr->stack_user );
		pushl( tr, EFLAG_BASE | EFLAG_INTERRUPT  );
		pushl( tr, GDT_USERCODE | 3 );
	}

	pushl( tr, entry );


	pushl( tr, GDT_USERDATA | 3 );	// gs
	pushl( tr, GDT_USERDATA | 3 );	// fs
	pushl( tr, GDT_USERDATA | 3 );	// ds
	pushl( tr, GDT_USERDATA | 3 );	// es

	pushl( tr, data1 );				// eax
	pushl( tr, data3 );				// ecx
	pushl( tr, data4 );				// edx
	pushl( tr, data2 );				// ebx

	pushl( tr, 0 );					// temp esp

	pushl( tr, 0 );					// ebp
	pushl( tr, 0 );					// esi
	pushl( tr, 0 );					// edi


	FAMILY_INIT( tr );
	LIST_INIT( tr );


	// ....

	if ( proc->threads == NULL )		// First thread!
	{
		proc->threads = tr;
	}
	else
	{
		if ( parent == NULL )	// Only happens in smk.
		{
			family_add_sibling( &(proc->threads->family_tree), 
								&(tr->family_tree) );
		}
		else
		{
			family_add_child( &(parent->family_tree), &(tr->family_tree) );
		}

		list_insertAfter( &(proc->threads->list), &(tr->list) );
	}

	htable_insert( proc->threads_htable, tr );

	proc->thread_count += 1;
	return tr->tid;
}


/** Deletes a thread entirely from the system. Everything is taken
 * care of. You can just delete a thread.
 *
 * \warning Oh yeah, it must belong to a process.
 *
 * \return 0 on success.
 */
int delete_thread( struct thread *tr )
{
	int rc;
	struct thread *parent;
	struct thread *next;

	assert( tr->process != NULL );
	
	tr->process->thread_count -= 1;


	rc = htable_remove( tr->process->threads_htable, tr );
	assert( rc == 0 );


	rc = release_thread_ports( tr );
	assert( rc >= 0 );
		


	// Save the children.
	parent = PARENT( tr, struct thread* );
	if ( parent != NULL )
	{
		// Give kids to the parent
		family_adopt( &(parent->family_tree), &(tr->family_tree) );
	}
	else
	{
		// Orphan them
		struct thread *child = CHILD( tr, struct thread* );
		while ( child != NULL )
		{
			family_remove( &(child->family_tree) );
			family_add_sibling( &(tr->family_tree), &(child->family_tree) );

			child = CHILD( tr, struct thread* );
		}
	}

	// Next
	next = NEXT_SIBLING( tr, struct thread* ); 


	family_remove( &(tr->family_tree) );		// Remove from tree
	list_remove( &tr->list );

	if ( tr->process->threads == tr )			// In case of first thread
	{
		// Assert to ensure the head of the list is always the head.
		assert( PREV_SIBLING( tr, void* ) == NULL );
		tr->process->threads = next;
	}


	// Now clean up the thread...
	
	tr->stack_kernel -= sK_PAGES_KERNEL * PAGE_SIZE;
	memory_free( sK_PAGES_KERNEL, (void*)tr->stack_kernel );

	tr->stack_user -= sK_PAGES_THREAD * PAGE_SIZE;
	page_map_out( tr->process->map, tr->stack_user, sK_PAGES_THREAD );
		
	memory_free( 1, tr->math );
	memory_free( 1, tr->tls );
	kfree( tr );
	return 0;
}



#include "include/cpu.h"
#include "include/tss.h"

extern uint32_t __switch_thread( uint32_t stack );


void set_switch_stack( uint32_t st )
{
	cpu[ CPUID ].scheduler_stack = st;
}

uint32_t get_switch_stack()
{
	return cpu[ CPUID ].scheduler_stack;
}



// -----------

int exec_thread( unsigned int cpu_id, struct thread *t, 
				 unsigned int milliseconds, int idle )
{
	assert( t != NULL );

	set_fpu_trap();

	set_map( t->process->map );

	cpu[ cpu_id ].sched.running = 1;			// Marked as running before
	cpu[ cpu_id ].sched.current_thread = t;		// Mark the thread.


	// ------------------------------------------------------------

	// ------------------------------------------------------------
	/*
	{
	int i = 0;
	int tmp = 0;


	((char*)0xb8000)[cpu_id * 160 + i * 2] = '0' + cpu[ cpu_id ].sched.sched_count; i++;
	((char*)0xb8000)[cpu_id * 160 + i * 2] = ' ' + cpu[ cpu_id ].sched.sched_count; i++;

	
	while ( t->process->name[i] != 0 )
	{
		((char*)0xb8000)[cpu_id * 160 + i * 2] = t->process->name[i];
		i++;
	}

	((char*)0xb8000)[cpu_id * 160 + i * 2] = '.';
	tmp = ++i;

	while ( t->name[i - tmp] != 0 )
	{
		((char*)0xb8000)[cpu_id * 160 + i * 2] = t->name[i - tmp];
		i++;
	}
	
		((char*)0xb8000)[cpu_id * 160 + i++ * 2] = ' ';
		((char*)0xb8000)[cpu_id * 160 + i++ * 2] = ' ';
		((char*)0xb8000)[cpu_id * 160 + i++ * 2] = ' ';
		((char*)0xb8000)[cpu_id * 160 + i++ * 2] = ' ';
	}
	*/

	// ---------------------------------------------------

	// Set the future interrupt
	if ( milliseconds != 0 ) set_apic_distance( cpu_id, milliseconds );

	cpu[ cpu_id ].sched.idle = idle;
		
	release_spinlock( &(cpu[ cpu_id ].sched.lock_scheduler) ); 
	// Other CPU's can register their need to mess with this CPU's tables.
	
		  	sysenter_set_esp( t->stack_kernel );
	
			cpu[ cpu_id ].system_tss->esp0 = t->stack_kernel;
			cpu[ cpu_id ].system_tss->esp  = t->stack;
			cpu[ cpu_id ].system_tss->cr3  = (uint32_t)t->process->map;
	
			stats_time( cpu_id, &(cpu[ cpu_id ].st_schedulerTime) ); // Scheduler time.

			t->stack = __switch_thread( t->stack );

			stats_time_start( &(cpu[ cpu_id ].st_schedulerTime) ); // Scheduler
		   
		   	// Safety check.
			assert_build( extern volatile int INSIDE_IRQ );
		    assert( (cpu_flags() & EFLAG_INTERRUPT) == 0 );
			assert( INSIDE_IRQ == 0 ); 

	// If math was used....
	if ( t->math_state > 0 ) save_fpu( t );
	
	cpu[ cpu_id ].sched.current_thread = NULL;
	cpu[ cpu_id ].sched.running = 0;
			// WARNING: Don't use the *t pointer anymore after this.

			// Synchronization point occurs here. Table messing. 

	acquire_spinlock( &(cpu[cpu_id].sched.lock_scheduler) );

	cpu[ cpu_id ].sched.idle = 0;

	ack_apic_timer();

	return 0;
}



int go_dormant( unsigned int timeout )
{
	struct process *proc;
	struct thread *tr;
	int duration;
	
	// First check if messages are waiting...
	proc = checkout_process( getpid(), WRITER );
	assert( proc != NULL );

		// Allowed to go dormant?
	    tr = current_thread();

		assert( (tr->state == THREAD_RUNNING) );
	
		if ( (pulses_waiting(tr) > 0) || (messages_waiting(tr) > 0) )
		{
			commit_process( proc );
			return 0;
		}
			
	// We're allowed to go dormant.
	
	    if ( timeout != 0 )
	    {
			disable_interrupts();
				pause( tr, timeout );
				commit_process( proc );
				//atomic_dec( &(proc->kernel_threads) );
				sched_yield();
				//atomic_inc( &(proc->kernel_threads) );
				duration = tr->duration;
				tr->duration = 0;
			enable_interrupts();
	    }
	    else
	    {
	
			disable_interrupts();
				tr->duration = 0;
				set_thread_state( tr, THREAD_DORMANT );
				commit_process( proc );
				atomic_dec( &(proc->kernel_threads) );
				sched_yield();
				atomic_inc( &(proc->kernel_threads) ); // But okay to die from here on out.
				duration = tr->duration;
				tr->duration = 0;
			enable_interrupts();
	    }


	return duration;
}



