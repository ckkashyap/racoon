#ifndef _KERNEL_THREADS_H
#define _KERNEL_THREADS_H

#include <smk/limits.h>
#include "include/gdt.h"


#define sK_BASE			0x80000000ul
#define sK_CEILING		0xFA000000ul
#define sK_PAGES_THREAD	10
#define sK_PAGES_KERNEL	2


#include <ipc/ipc.h>

#include <ds/family.h>
#include <ds/list.h>
#include <ds/htable.h>

struct process;			// Just declare it.

#define THREAD_SYSTEM			0
#define THREAD_RUNNING			1
#define THREAD_SUSPENDED		2
#define THREAD_DORMANT			3
#define THREAD_STOPPED			4
#define THREAD_SEMAPHORE		5
#define THREAD_SLEEPING			6
#define THREAD_WAITING			7
#define THREAD_FINISHED			8
#define THREAD_CRASHED			9
#define THREAD_IRQ				10




struct thread
{
   char name[SMK_NAME_LENGTH];		// thread name
   int tid;								// thread id
   int state;							// thread state

   struct process *process;				// The process

   int irq;				// IRQ number if this is a handler
   
   uint64_t	duration;	// Time spent sleeping or expecting to sleep
   

   int	event_tid;						// The handling tid of events
   
   int rc;				// return code.


	void*	tls;	// thread local storage value

/*
   int32 priority;



   unsigned int capabilities[ TOTAL_SYSCALLS ];	// syscall permission table
  
*/
   
   int ports[ MAX_PORTS ];		///< The ports currently in use by the thread	
   int last_port_pulse;
   int last_port_message;

/*
   uint32 sleep_seconds;
   uint32 sleep_milliseconds;
 

   struct wait_info *waits;		// other threads waiting on this
   struct wait_info *active_wait;	// this wait information.
   

   uint32 stack_kernel;
   uint32 stack_user;
   uint32 stack_pages;
*/
   
   int cpu;					// The CPU that this thread is on.
   int cpu_affinity;		// The CPU that this thread prefers.
   int fast_queued;			// The count of fast queues pending
   int interrupts_disabled; // Should be set to 1 if interrupts are disabled
   
   uint32_t  stack_kernel;
   uint32_t  stack_user;
   uint32_t  stack;

   int math_state;
   uint8_t *math; 			// fpu,mmx,xmm,sse,sse2

   FAMILY_TREE;
   LINKED_LIST;
   
};



int new_thread( int type,
	       	    struct process *proc, 
		   	    struct thread *parent, 
		        const char name[SMK_NAME_LENGTH],
			    uint32_t entry,
			    int priority,
			    uint32_t data1,
			    uint32_t data2,
			    uint32_t data3,
			    uint32_t data4 );

int delete_thread( struct thread *tr );


struct thread* get_thread( struct process *proc, int tid );

int exec_thread( unsigned int cpu_id, struct thread *t, 
				 unsigned int milliseconds,
				 int idle );

int go_dormant( unsigned int timeout );

int threads_key( void *td );
int threads_compare( void *ad, void *bd );


void set_thread_state( struct thread *t, unsigned int state );


#endif

