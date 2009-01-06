#include <smk/strings.h>
#include <smk/inttypes.h>
#include <smk/process_info.h>
#include <smk/atomic.h>
#include <mm/mm.h>

#include "process.h"
#include "include/misc.h"
#include "include/ualloc.h"

#include "include/paging.h"


#include <ds/htable.h>
#include <ds/list.h>
#include <ds/rwlock.h>
#include <mm/vmem.h>

#include "include/env.h"


static struct hash_table *l_processTable = NULL;	///< Table for all pids
static struct process 	 *l_rootProcess  = NULL;	///< Root process
static struct rwlock 	 *l_processLock  = NULL;	///< RW lock for processes


static int l_PID = 1;							///< PID counter.
static spinlock_t l_PIDlock	= INIT_SPINLOCK;	///< PID lock.



// -------------------------    hash table key and compare functions.

int ptable_key( void *d )
{
	struct process *p = (struct process*)d;
	int ans = p->pid * 100;
	if ( ans < 0 ) ans = -ans;
	return ans;
}

int ptable_compare( void* ad, void* bd )
{
	struct process *a = (struct process*)ad;
	struct process *b = (struct process*)bd;

	if ( (b->pid != -1) )
	{
		if ( a->pid == b->pid ) return 0; 
		if ( a->pid < b->pid ) return -1;
		return 1;
	}
	
	return kstrncmp( a->name, b->name, SMK_NAME_LENGTH );
}


// --------------------------------------------------------


int next_pid()
{
	int pid;
	struct process temp;
				   temp.name[0] = 0;
				   temp.pid = -1;

	acquire_spinlock( &(l_PIDlock) );
		
		do
		{
			if ( temp.pid != -1 )
			{
				l_PID += 1;
				if ( l_PID < 0 ) l_PID = 2;
			}
	
			temp.pid = l_PID;
		}
		while ( htable_retrieve( l_processTable, &temp ) != NULL );

		pid = l_PID++;

	release_spinlock( &(l_PIDlock) );

	return pid;
}


// --------------------------------------------------------

void init_processes()
{
	l_processTable = init_htable( 128, 75, ptable_key, ptable_compare );
	l_processLock = init_rwlock();

	assert( l_PID == 1 );
}



struct process *new_process( const char name[SMK_NAME_LENGTH] )
{
	int i;
		
	struct process *proc = kmalloc( sizeof(struct process) );
	if ( proc == NULL ) return NULL;

	kstrncpy( proc->name, name, SMK_NAME_LENGTH );

	proc->pid = next_pid();
	proc->state = PROCESS_INIT;

	proc->map = new_page_directory();

	proc->virtual_memory = vmem_create( "virtual_memory",
								(void*)0x80000000,
								0x7FFFF000,
								0x1000,
								NULL,
								NULL,
								NULL,
								0x10000,
								0 );


	proc->lock = init_rwlock();

	proc->threads = NULL;
	proc->threads_htable = init_htable( 128, 75, threads_key, threads_compare );   
	proc->thread_count = 0;

	proc->last_tid = 1;

	proc->event_hooks = 0;
	proc->event_pid	= -1;
	proc->event_tid	= -1;

	proc->rc 		= 0;

	proc->kernel_threads = 0;

	proc->flags = 0;

	proc->environment = NULL;
	proc->environment_count = 0;

	proc->alloc = NULL;


	// Create IPC block.
		for ( i = 0; i < MAX_PORTS; i++ )
		{
			proc->ipc[i].tid 			= -1;
			proc->ipc[i].flags 			= 0;

			proc->ipc[i].pulse_q		= NULL;
			proc->ipc[i].message_q		= NULL;
		}
	


	FAMILY_INIT( proc );
	LIST_INIT( proc );
	return proc;
}


void delete_process( struct process* proc )
{
	user_release_all( proc );

#warning event system?
#warning shared memory release

	delete_rwlock( proc->lock );
	release_environment( proc );
	delete_htable( proc->threads_htable );

	vmem_destroy( proc->virtual_memory );

	delete_page_directory( proc->map );
	kfree( proc );
}




int find_process( const char name[SMK_NAME_LENGTH] )
{
	int pid = -1;
	struct process *tmp = NULL;
		
	rwlock_get_read_access( l_processLock );


		tmp = l_rootProcess;

		while ( tmp != NULL )
		{
			if ( kstrncmp( name, tmp->name, SMK_NAME_LENGTH ) == 0 )
			{
				pid = tmp->pid;
				break;
			}

			tmp = NEXT_LINEAR( tmp, (struct process*) );
		}
	

	rwlock_release( l_processLock );
	return pid;
}


int insert_process( int pid, struct process *proc )
{
	struct process *parent = NULL;
	struct process temp;

	if ( l_rootProcess == NULL )
	{
		l_rootProcess = proc;
		htable_insert( l_processTable, proc );
		return proc->pid;		
	}

	assert( pid >= 0 );

	// Get the parent process.
		temp.name[0] = 0;
		temp.pid = pid;
	
	
	rwlock_get_write_access( l_processLock );		// We are writing the process list.
	
		parent = (struct process*)htable_retrieve( l_processTable, &temp );
		assert( parent != NULL );

	
		// Insertion.
		family_add_child( &(parent->family_tree), &(proc->family_tree) );
		list_insertAfter( &(l_rootProcess->list), &(proc->list) );
		htable_insert( l_processTable, proc );

		temp.pid = proc->pid;

	rwlock_release( l_processLock );

	return temp.pid;
}

void remove_process( struct process *proc )
{
	struct process *parent;
		
	rwlock_get_write_access( l_processLock );		// We are writing the process list.

		parent = PARENT( proc, struct process* );
		assert( parent != NULL );		// smk is always the parent
		family_adopt( &(parent->family_tree), &(proc->family_tree) );


		family_remove( &(proc->family_tree) );		// Remove from tree
		list_remove( &proc->list );
		htable_remove( l_processTable, proc );
	  
	rwlock_release( l_processLock );
}



// --------------------------------------------------------------


/** Acquires a rwlock on the global process list and on the
	process requested, if found. 
	
	flags = READER | WRITER
*/

struct process *checkout_process( int pid,  unsigned int flags )
{
	struct process temp;
	struct process *p;

		  temp.pid = pid;
		  temp.name[0] = 0;

//	if ( (getpid() == 4) || (pid == 4) )
//		dmesg("%!(%i)Attempting to lock %i\n", getpid(), pid );

	rwlock_get_read_access( l_processLock );		// We are reading.

		p = ((struct process*)htable_retrieve( l_processTable, &temp ));

		if ( p == NULL )
		{
			rwlock_release( l_processLock );
			return NULL;
		}

//	dmesg("%!(%i)Locking %s by %i ( %i )\n", getpid(),
//					p->name, getpid(), flags );

	if ( (flags == READER) ) rwlock_get_read_access( p->lock );
						else rwlock_get_write_access( p->lock );
	
	// Leave the global process list locked to ensure data integrity.
	
//	if ( (getpid() == 4) || (pid == 4) )
//	dmesg("%!(%i.%i)Locked %i\n", 
//					getpid(), 
//					gettid(),  
//				    pid );
						
	return p;
}

/** Release the global process list - in case the original process
 * stopped existing */
void			release_processes()
{
	rwlock_release( l_processLock );
}

/** Release a process and the global process list.  */

void 			commit_process( struct process *p )
{
//	dmesg("%!(%i)Freeing %s(%i)\n", getpid(), p->name, p->pid );
		
	rwlock_release( p->lock );
	rwlock_release( l_processLock );

//	if ( (getpid() == 4) || (p->pid == 4) )
//	dmesg("%!(%i.%i)Freed %i\n", 
//					getpid(), 
//					gettid(), 
//					p->pid );
}




/** This is an easy method to parse the process list while keeping
 * the whole thing locked, safe, sane, etc.
 *
 * If the callback function returns anything else besides 0, then
 * the callback processing loop ends.
 */
int 	process_foreach( process_callback callback, unsigned int flags )
{
	struct process *p;
	int ans = 0;
	
	if ( (flags == READER) ) rwlock_get_read_access( l_processLock );
	if ( (flags == WRITER) ) rwlock_get_write_access( l_processLock );

		p = l_rootProcess;
		while ( p != NULL )
		{
			ans = callback( p );
			if ( ans != 0 ) break;
			p = NEXT_LINEAR( p, NULL );
		}
		
	
	rwlock_release( l_processLock );
	return ans;
}


int		process_get_list( int max,  struct process_information* list )
{
	int	done = 0;

	int internal_callback( struct process *p )
	{
		size_t	size = SMK_CMDLINE_LENGTH;
		int rc = 0;
		kstrncpy( list[done].name, p->name, SMK_NAME_LENGTH );

		rc = get_environment( p, "CMD_LINE", list[done].cmdline, &size );
		assert( rc >= 0 );

		list[ done ].pid 	= p->pid;
		list[ done ].state 	= p->state;

		if ( ++done == max ) return -1;
		return 0;
	}

	process_foreach( internal_callback, WRITER );

	return done;
}












