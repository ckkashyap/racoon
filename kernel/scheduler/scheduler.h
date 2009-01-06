#ifndef _KERNEL_SCHEDULER_H
#define _KERNEL_SCHEDULER_H

#include <smk/atomic.h>

#define MAX_FAST		32

struct thread;

			

struct schedule
{
	struct thread *tr;
};

struct paused_schedule
{
	struct thread *tr;
	int state;
	uint64_t request;
	uint64_t start;
};

struct scheduler_info
{
	unsigned int cpu_id;

	struct thread *current_thread;

	int requesting_state;
	struct thread *requesting_thread;
	
		
	struct schedule* sched_queue;
	struct paused_schedule* pause_queue;
	volatile unsigned int sched_count;
	volatile unsigned int pause_count;
	volatile unsigned int sched_size;
	volatile unsigned int pause_size;

	volatile unsigned int running;
	volatile unsigned int idle;

 	spinlock_t lock_scheduler;
	volatile unsigned int lock_flags;
	volatile unsigned int nested_lock;	// For debug purposes. Remove one day

	volatile unsigned int position;


	struct thread *fast_queue[ MAX_FAST ];
	volatile unsigned int fast_start;
	volatile unsigned int fast_stop;

};



void load_scheduler( unsigned int cpu_id );

int queue( struct thread *tr );
int dequeue( struct thread *tr );
int pause( struct thread *tr, uint64_t milliseconds );
int restart( struct thread *tr );

int fast_queue( struct thread *tr );
int fast_dequeue( struct thread *tr );

#include <include/apic.h>

static inline void sched_yield()
{
	asm ( "int %0" : : "g" (APIC_INTERRUPT) );
}

void sched_lock( int cpu_id, int complete );
void sched_unlock( int cpu_id );

void sched_lock_all();
void sched_unlock_all();

#ifndef _KERNEL_CPU_H
#include <include/cpu.h>
#endif

/// \todo MACROS should not be in lower case

#define current_thread() cpu[ CPUID ].sched.current_thread
#define current_process() cpu[ CPUID ].sched.current_thread->process


#define getpid() cpu[ CPUID ].sched.current_thread->process->pid
#define gettid() cpu[ CPUID ].sched.current_thread->tid


#endif

