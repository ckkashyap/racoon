#ifndef _KERNEL_PROCESS_H
#define _KERNEL_PROCESS_H

#include <smk/limits.h>
#include <smk/inttypes.h>
#include <smk/kernel_info.h>
#include <smk/process_info.h>


#include <processes/threads.h>

#include <ds/rwlock.h>
#include "include/stats.h"
#include <ipc/ipc.h>

#include <ds/list.h>
#include <ds/htable.h>
#include <ds/family.h>

#include <mm/vmem.h>


struct process
{
   char name[SMK_NAME_LENGTH];	// process name
   unsigned int pid;					// process id
   int state;							// state of the process

   uint32_t* map;						// virtual memory map location
   vmem_t*	virtual_memory;				// virtual memory map

   struct rwlock *lock;					// rwlock for access to process.

   struct thread *threads;	  		 	// Threads belonging to the process.
   struct hash_table *threads_htable;	// Hash table of process' threads.
   int thread_count;					// number of threads in process
   
   int last_tid;						// The last tid used
   

   unsigned int kernel_threads;			// # of threads in the kernel.

   unsigned int flags;					// Flag information about the process.
   

   volatile int	event_hooks;			// The number of event handlers
   int	event_pid;						// The handling pid of events
   int	event_tid;						// The handling tid of events
   

   int rc;								// return code of the process
   /*
   void **tls_location;					// location of process tls.
   */
 
   void* environment;					// Environment variables. 
   int   environment_count;				// Environment size;
	
   /*
   uint64_t			st_startTime;		// Start time in milliseconds
   struct s_time	st_runningTime;		// Amount of time on the CPU
   struct s_time	st_syscallTime;		// Amount of time within syscalls
   */
		 

   struct allocated_memory *alloc; 	// Linked list of allocated memory

   struct ipc_block ipc[MAX_PORTS];	// IPC ports.
   /*
   struct shared_memory *shared;   	// Linked list of shared memory
   struct semaphore *sems;	   	// Linked list of processes' semaphores
   spinlock sems_lock;			// Semaphore lock.
   struct wait_info *waits;		// Linked list of waiting pid/tid's
   */

   FAMILY_TREE;
   LINKED_LIST;
};



struct process *new_process( const char name[SMK_NAME_LENGTH] );
void delete_process( struct process *proc );


int find_process( const char name[SMK_NAME_LENGTH] );


int insert_process( int pid, struct process *proc );
void remove_process( struct process *proc );


void init_processes();


struct process *checkout_process( int pid,  unsigned int flags );
void 			commit_process( struct process *proc );
void			release_processes();


typedef int (*process_callback)(struct process*);
int 	process_foreach( process_callback callback, unsigned int flags );

int		process_get_list( int max,  struct process_information* list );


#endif


