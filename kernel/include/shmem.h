#ifndef _KERNEL_SHMEM_H
#define _KERNEL_SHMEM_H

#include <smk/inttypes.h>
#include <smk/limits.h>
#include <processes/process.h>
#include <ds/rwlock.h>

#include <ds/list.h>


#define SHMEM_RDWR			1		///< region writable
#define SHMEM_IMMORTAL		2		///< persists after owner's death
#define SHMEM_PROMISCUOUS	4		///< allows anyone to request.

#define SHMEM_ORPHAN		32		///< Indicates whether the owner is dead.


#define SHMEM_MASK			(1|2|4)	///< Mask for all the valid SHMEM flags that userland can set.

/** A structure detailing which application the shared memory region
 * is mapped into.
 */
struct shmem_info
{
	int pid;						///< PID that is using this shmem
	uintptr_t virtual_location;		///< Virtual location of physical location.
	int virtual_pages;				///< Number of virtual pages mapped in.
	unsigned int flags;				///< Read only, read-write, etc

	int usage;						///< Number of times it has been requested.

	void *tmp;						///< Small tmp variable for easy deletion.
	void *tmp2;						///< Small tmp variable for easy deletion.

	LINKED_LIST;					///< The linked list.
};


/** The master structure detailing the exact shared memory information */
struct shmem
{
	int id;								///< Shared memory ID.
	char name[ SMK_NAME_LENGTH ];	///< Shared memory name.

	struct rwlock *lock;				///< The reader-writer lock.
	
	int pid;							///< Shared memory owner.
	unsigned int flags;					///< Flags detailing the shared memory. 
	unsigned int internal;				///< Specified whether the memory was allocated internally and should be freed.

	uintptr_t 	physical_location;		///< Physical location.
	int  	   	physical_pages;			///< Number of pages of physical location.
	
	struct shmem_info *list;			///< The list of users.	
};




int shmem_init();
int shmem_shutdown();


int shmem_create( const char name[ SMK_NAME_LENGTH ], 
				  int pid, 
				  int pages, 
				  unsigned int flags );

int shmem_create_direct(  const char name[ SMK_NAME_LENGTH ], 
						  int pid, 
						  int pages, 
						  unsigned int flags,
						  uintptr_t location,
						  unsigned int internal );
				
int shmem_grant( int owner, 
				 int id, 
				 int pid, 
				 unsigned int flags );

int shmem_revoke( int owner, 
				  struct process *proc, 
				  int id );
				
int shmem_request( 	struct process *proc, 
					int id, 
					void **location, 
					int *pages, 
					unsigned int *flags );

int shmem_release( struct process *proc, 
				int id );

int shmem_delete( int owner, 
				int id );

int shmem_clean( int pid );


int shmem_find( const char name[ SMK_NAME_LENGTH ], 
				int *pid );

int shmem_get( int i, 
				char name[ SMK_NAME_LENGTH], 
				int *pid, 
				int *pages, 
				unsigned int *flags );


#endif









