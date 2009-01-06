#include <smk/strings.h>

#include "include/dmesg.h"
#include "include/shmem.h"
#include "include/paging.h"

#include "include/misc.h"
#include "mm/physmem.h"


#include <ds/htable.h>


static struct hash_table *shmem_table = NULL; 	///< This is the shared memory hash table. It is hashed according to shared memory id.
static int shmem_id	= 0;						///< This is the ID tracker. Should be equal to the last ID used + 1.
static struct rwlock *shmem_lock = NULL;  		///< The rwlock used to protect shmem_table.



/** The htable key function. It's very simple. */
int shmem_key( void *a )
{
	struct shmem *sh = (struct shmem*)a;
	return (sh->id * 100);
}

/** The htable comparison function. It's also very simple. */
int shmem_cmp( void *a, void *b )
{
	struct shmem *sa = (struct shmem*)a;
	struct shmem *sb = (struct shmem*)b;

	if ( sa->id == sb->id ) return 0;
    if ( sa->id < sb->id ) return -1;

	return 1;
}



									 
/** Creates the shared memory hash tables and initializes all the important stuff. */
int shmem_init()
{
	// An initial count of 64 slots with an 80% rehash limit.
	shmem_table = init_htable( 64, 80, shmem_key, shmem_cmp );
	shmem_id = 0;
	shmem_lock = init_rwlock();
	dmesg("initialized the shmem structures.\n");
	return 0;
}


int shmem_shutdown()
{
	assert( (0!=0) );
		
	delete_rwlock( shmem_lock );
	return 0;
}



/** Allocates a region of memory and bounces into shmem_create_direct.
 *
 * \return -1 if failure.
 * \return 0 or greater - representing the ID of the memory region. 
 */
int shmem_create( const char name[ SMK_NAME_LENGTH ], 
					int pid, 
					int pages, 
					unsigned int flags )
{
	int id;

	uintptr_t location = (uintptr_t)memory_alloc( pages );
	if ( location == 0 ) return -1;

	id = shmem_create_direct( name, pid, pages, flags, location, 1 );
	if ( id < 0 )
		memory_free( pages, (void*)location );

	return id;
}


/** Creates a new shared memory structure in memory with the given name and pid of the
 * given page sizes. The flags are applied to the structure and all future operations
 * on the shared memory region. The physical memory location is specified in the 
 * paramters.
 *
 * \return -1 if failure.
 * \return 0 or greater - representing the ID of the memory region. 
 */
int shmem_create_direct( const char name[ SMK_NAME_LENGTH ], 
					int pid, 
					int pages, 
					unsigned int flags,
					uintptr_t location,
					unsigned int internal )
{
	struct shmem sample;
	struct shmem *sh;

	//dmesg("%!create_shmem_direct: %s, %i, %i, %x, %x, %i\n", name, pid, pages, flags, location, internal);
	
	// Pointless if there's no pages.
	if ( pages <= 0 ) return -1;

#warning This should not really be done, dude
	if ( kstrlen( name ) <= 0 ) return -1; 
	
	// Create the structure immediately	
	sh = (struct shmem*)kmalloc( sizeof( struct shmem ) );
	if ( sh == NULL ) return -1;

	// Change the information
		sh->pid = pid;
		sh->physical_location = location;
		sh->physical_pages = pages;
		sh->flags = (flags & SHMEM_MASK);
		sh->internal = internal;
		sh->list = NULL;
		sh->lock = init_rwlock();
		kstrncpy( sh->name, name, SMK_NAME_LENGTH );


		rwlock_get_write_access( shmem_lock );

			do
			{
				sample.id = shmem_id++;
				if ( shmem_id < 0 )	 shmem_id = 0;	// Roll over.
			} 
			while ( htable_retrieve( shmem_table, &sample ) != NULL );
	

			sh->id = sample.id;
			htable_insert( shmem_table, sh );

		
		rwlock_release( shmem_lock );

	// Return the ID.
	return sample.id;
}


/** Grants access to the shared memory to the given pid. It does
 * not map in the shared region. It just marks the pid as valid.
 *
 * Only the owner can grant access to the shared memory region.
 * 
 * \return 0 if the grant was successful.
 */ 
int shmem_grant( int owner, int id, int pid, unsigned int flags )
{
	struct shmem *sh;
	struct shmem_info *info;
	struct shmem sample;
				 sample.id = id;

	//dmesg("%!grant_shmem: %i, %i, %x\n", id, pid, flags );
				 
	rwlock_get_read_access( shmem_lock );
		
		sh = htable_retrieve( shmem_table, &sample );
		if ( sh == NULL ) 
		{
			rwlock_release( shmem_lock );
			return -1;
		}

	// Search the list and provide info if required.

		rwlock_get_write_access( sh->lock );

			// Is the owner requesting this?
			if ( owner != sh->pid )
			{
				rwlock_release( sh->lock );
				rwlock_release( shmem_lock );
				return -1;
			}
		
			// .. already present?
		
			info = sh->list;
			while ( info != NULL )
			{
				if ( info->pid == pid ) 
				{
					rwlock_release( sh->lock );
					rwlock_release( shmem_lock );
					return -1;
				}
	
				info = NEXT_LINEAR( info, struct shmem_info* );
			}
		
		
			// .. create a new info.
		
			info = (struct shmem_info*)kmalloc( sizeof(struct shmem_info) );
			if ( info == NULL )
			{
				rwlock_release( sh->lock );
				rwlock_release( shmem_lock );
				return -1;
			}
			
			info->pid = pid;
			info->virtual_location = 0;
			info->virtual_pages = -1;
			info->flags = (flags & SHMEM_MASK);
			info->tmp = NULL;
			info->tmp2 = NULL;
			info->usage = 0;

			// insert.

			LIST_INIT( info );

			if ( sh->list != NULL )
				list_insertBefore( &(sh->list->list), &(info->list) );

			sh->list = info;
		

		rwlock_release( sh->lock );
	rwlock_release( shmem_lock );
	return 0;
}



/** This function will take the shared memory identified as ID
 * and revokes all access with regards to PID. If there is memory
 * mapped into the pid, wrg to this ID, then the actual pages
 * are duplicated.  That way, one process can not cause another
 * process to page fault by revoking in the middle of the
 * other doing something.
 *
 * Only the owner may revoke access.
 * 
 */
int shmem_revoke( int owner, struct process *proc, int id )
{
	struct shmem *sh;
	struct shmem_info *info;
	struct shmem sample;
				 sample.id = id;
	uint32_t tmp;

	//dmesg("%!revoke_shmem: %i, %i\n", proc->pid, id );
				 
	rwlock_get_read_access( shmem_lock );
		
		sh = htable_retrieve( shmem_table, &sample );
		if ( sh == NULL ) 
		{
			rwlock_release( shmem_lock );
			return -1;
		}

	// Search the list and provide info if required.

		rwlock_get_write_access( sh->lock );
		

			// Is the owner requesting this?
			if ( owner != sh->pid )
			{
				rwlock_release( sh->lock );
				rwlock_release( shmem_lock );
				return -1;
			}
		
			// .. already present?
	
			info = sh->list;
			while ( info != NULL )
			{
				if ( info->pid == proc->pid ) break; 
				info = NEXT_LINEAR( info, struct shmem_info* );
			}
		
			if ( info == NULL )
			{
				rwlock_release( sh->lock );
				rwlock_release( shmem_lock );
				return -1;
			}
			

			// If this region is still being used, then we need
			// to provide a fake region of memory.
			if ( info->usage > 0 )
			{
				int enabled = disable_interrupts();
	
				/// \todo some memory gets stuck in the process map.
					// Some thread is still using the area. Just give it a scratch pad.
					void *data = memory_alloc( sh->physical_pages );
					if ( data == NULL )
					{
						rwlock_release( sh->lock );
						rwlock_release( shmem_lock );
						return -1;
					}
						
					// Maintain the previous permissions.
					if ( (info->flags & SHMEM_RDWR) == SHMEM_RDWR )
							tmp = USER_PAGE | READ_WRITE;
					else
							tmp = USER_PAGE | READ_ONLY;

				disable_interrupts();	// Make the fake, safely.
					
					// Map in the fake memory.
					page_direct_map( proc->map,
									 (uintptr_t)data,
									 info->virtual_location,
									 info->virtual_pages,
									 tmp );

					// Copy the old data across.
					kmemcpy( data, (void*)sh->physical_location, sh->physical_pages * PAGE_SIZE );
				
				if ( enabled == 1 ) enable_interrupts();
			}
			
			
			// unlink
			if ( sh->list == info ) sh->list = NEXT_LINEAR( info, struct shmem_info* );
			list_remove( &(info->list) );
			kfree( info );
			

		rwlock_release( sh->lock );
	rwlock_release( shmem_lock );
	return 0;
}




				
/** The given process is requesting access to the shared memory
 * region identified as ID id. If it's a successful request,
 * *location will point to the virtual location in proc's process
 * map that *pages amount of pages are mapped in. *flags will
 * return the type of memory given.
 *
 *
 * \return 0 on successful access.
 * \return -1 on failure.
 */
int shmem_request( struct process *proc, 
					int id, 
					void **location, 
					int *pages, 
					unsigned int *flags )
{
	struct shmem *sh;
	struct shmem_info *info;
	struct shmem sample;
				 sample.id = id;
	uint32_t tmp;

	//dmesg("%!request_shmem: %i, %i\n", proc->pid, id );
				 
	rwlock_get_read_access( shmem_lock );
		
		sh = htable_retrieve( shmem_table, &sample );
		if ( sh == NULL ) 
		{
			rwlock_release( shmem_lock );
			return -1;
		}

	// Search the list and provide info if required.

		rwlock_get_write_access( sh->lock );
		
			// .. already present?
	
			info = sh->list;
			while ( info != NULL )
			{
				if ( info->pid == proc->pid ) break; 
				info = NEXT_LINEAR( info, struct shmem_info* );
			}
		
			// not present, but is it promiscuous?
		
			if ( (info == NULL) && ( (sh->flags & SHMEM_PROMISCUOUS) == SHMEM_PROMISCUOUS ) )
			{
				info = (struct shmem_info*)kmalloc( sizeof(struct shmem_info) );
				if ( info == NULL )
				{
					rwlock_release( sh->lock );
					rwlock_release( shmem_lock );
					return -1;
				}
				
				info->pid = proc->pid;
				info->virtual_location = 0;
				info->virtual_pages = -1;
				info->flags = (sh->flags & SHMEM_MASK);
				info->usage = 0;
				info->tmp = NULL;
				info->tmp2 = NULL;
	

				// insert?
				LIST_INIT( info );

				if ( sh->list != NULL ) 
					list_insertBefore( &(sh->list->list), &(info->list) );

				sh->list = info;
			}
		
			// Is there an info now?

			if ( info == NULL )
			{
				// Nope, dude. Not allowed.
				rwlock_release( sh->lock );
				rwlock_release( shmem_lock );
				return -1;
			}
			

			// Now we are sure that an info exists and we are pointing to it.

			if ( info->virtual_pages < 0 )
			{
				if ( (info->flags & SHMEM_RDWR) == SHMEM_RDWR )
						tmp = USER_PAGE | READ_WRITE;
				else
						tmp = USER_PAGE | READ_ONLY;


					
				// Map in.
				info->virtual_location = page_provide( proc->map,
					  								   sh->physical_location,
									 				  	sK_BASE,
									  				 	sK_CEILING,
									   					sh->physical_pages,
									   					tmp );

				// Failure to map.
				if ( info->virtual_location == 0 )
				{
					rwlock_release( sh->lock );
					rwlock_release( shmem_lock );
					return -1;
				}

				info->virtual_pages = sh->physical_pages;
			}
			
			

			info->usage += 1;

			*location = (void*)info->virtual_location;
			*pages = info->virtual_pages;
			*flags = info->flags;
			

		rwlock_release( sh->lock );
	rwlock_release( shmem_lock );
	return 0;
}



/** Release the shared memory from the given process. Note
 * that this won't necessarily release the paged memory because
 * it may have been requested multiple times by multiple threads.
 *
 * To have the memory released, this needs to be called multiple
 * times until usage == 0.
 * 
 */
int shmem_release( struct process *proc, int id )
{
	struct shmem *sh;
	struct shmem_info *info;
	struct shmem sample;
				 sample.id = id;

	//dmesg("%!release_shmem: %i, %i\n", proc->pid, id );
				 
	rwlock_get_read_access( shmem_lock );
		
		sh = htable_retrieve( shmem_table, &sample );
		if ( sh == NULL ) 
		{
			rwlock_release( shmem_lock );
			return -1;
		}

	// Search the list and provide info if required.

		rwlock_get_write_access( sh->lock );
		
			// .. already present?
	
			info = sh->list;
			while ( info != NULL )
			{
				if ( info->pid == proc->pid ) break; 
				info = NEXT_LINEAR( info, struct shmem_info* );
			}
		
			// not present!! Silly userland.
			if ( info == NULL) 
			{
				rwlock_release( sh->lock );
				rwlock_release( shmem_lock );
				return -1;
			}
		
			// Now we are sure that an info exists and we are pointing to it.

			info->usage -= 1;
			
			if ( info->usage == 0 )
			{
	 			page_release( proc->map, info->virtual_location, info->virtual_pages ); 
				info->virtual_location = 0;
				info->virtual_pages = -1;
			}
			
			

		rwlock_release( sh->lock );
	rwlock_release( shmem_lock );
	return 0;
}


/** This will search the shared memory list and will return
 * the ID of the shared memory region which has a name
 * matching the given name.
 *
 * return -1 if not found.
 */
int shmem_find( const char name[ SMK_NAME_LENGTH ], int *pid )
{
	int i = 0;
	struct shmem *sh;

	//dmesg("%!find_shmem: %s\n", name );

	rwlock_get_read_access( shmem_lock );
	

	while ( (sh = htable_get( shmem_table, i++ ) ) != NULL )
	{
		if ( kstrncmp( name, sh->name, SMK_NAME_LENGTH ) == 0 )
		{
			i = sh->id;
			*pid = sh->pid;
			rwlock_release( shmem_lock );
			return i; 
		}
		
	}


	rwlock_release( shmem_lock );
	return -1;
}



/** Returns information about the n'th shared memory region
 * in the list. Be advised that the return values will not
 * be sorted in any way. They will be first-seen, first
 * reported and the order will change depending on the
 * number of inserts and removes from the hash table.
 */
int shmem_get( int num, char name[ SMK_NAME_LENGTH], 
						  int *pid, int *pages, 
						  unsigned int *flags )
{
	int i = 0;
	struct shmem *sh;

	//dmesg("%!get_shmem: %i\n", i );

	rwlock_get_read_access( shmem_lock );
	

	while ( (sh = htable_get( shmem_table, i++ ) ) != NULL )
	{
		if ( i > num )
		{
			kstrncpy( name, sh->name, SMK_NAME_LENGTH );
			*pid = sh->pid;
			*pages = sh->physical_pages;
			*flags = sh->flags;
			rwlock_release( shmem_lock );
			return 0; 
		}
		
	}


	rwlock_release( shmem_lock );
	return -1;
}



/** Delete the shared memory with the ID id. The flags parameter
 * determines what happens to the shared memory should anything
 * unusual be the case.
 *
 * Only the owner can delete a shared region
 *
 *
 * \warning You can not currently delete shared memory that is
 * in use. This is wrong. It should map in a scratch pad memory
 * region.
 * 
 * \warning THIS NEEDS TO MODIFY THE MEMORY MAPS OF SEVERAL PROCESSES.
 * 			(if they're using the shared memory region). So, this
 * 			function expects the system to be fully functional so
 * 			that it can lock each process for writing.
 *
 * 			It brings the system to a nice, clean halt.. basically.
 * 
 * \warning This is a complicated function. Have some coffee first.
 * 
 */
int shmem_delete( int owner, int id )
{
	int usage = 0;				// The number of users still left.
		
	struct shmem *sh;
	struct shmem_info *info;
	struct shmem sample;
				 sample.id = id;
				 

	//dmesg("%!delete_shmem: %i %i\n", owner, id );
				 
	rwlock_get_write_access( shmem_lock );
		
		sh = htable_retrieve( shmem_table, &sample );
		if ( sh == NULL ) 
		{
			rwlock_release( shmem_lock );
			return -1;
		}

			// Is the owner requesting this?
			if ( owner != sh->pid )
			{
				rwlock_release( shmem_lock );
				return -1;
			}

			// Remove it from the shared memory list.
			if ( htable_remove( shmem_table, sh ) != 0 )
			{
				assert( (1==0) );
			}

	rwlock_release( shmem_lock );
	// And now nothing is locked.

	// According the the system, the shared memory region no longer exists.

	// Now provide scratch areas to this processes which are still using the region.

			// First, check all the important ones out for writing!
		info = sh->list;
			
		while ( info != NULL )
		{
			if ( info->usage > 0 ) 
			{
				info->tmp = checkout_process( info->pid, WRITER );
				if ( info->tmp != NULL ) 
				{
					// Create this one's scratch pad.
					info->tmp2 = (void*)memory_alloc( sh->physical_pages );
					usage += 1;		// Got one!
				}
			}
				
			info = NEXT_LINEAR( info, struct shmem_info* );
		}


			// If there are users, do them all.
			// It must be done this way to ensure that the data is copied immediately
			// between all processes.
		if ( usage > 0 )
		{
			int enabled = disable_interrupts();		// Careful now..

				// Scratch pad is now consistent.

				info = sh->list;
				while ( info != NULL )
				{
					if ( (info->tmp != NULL) && (info->tmp2 != NULL) )
					{
						struct process *proc = (struct process*)info->tmp;
						unsigned int tmp;

						kmemcpy( info->tmp2, (void*)sh->physical_location, sh->physical_pages * PAGE_SIZE );
						

						// Maintain the previous permissions.
						if ( (info->flags & SHMEM_RDWR) == SHMEM_RDWR )
							tmp = USER_PAGE | READ_WRITE;
						else
							tmp = USER_PAGE | READ_ONLY;

						// Map in the fake memory.
							page_direct_map( proc->map,
											 (uintptr_t)(info->tmp2),
											 info->virtual_location,
											 sh->physical_pages,
											 tmp );
					}
					

					info = NEXT_LINEAR( info, struct shmem_info* );
				}


			if ( enabled == 1 ) enable_interrupts();
		}
		
	
		// Now release all processes and free the information.
		info = sh->list;
			
		while ( info != NULL )
		{
			void *p = info;
			if ( info->tmp != NULL ) commit_process( (struct process *) info->tmp ); 
			kfree( p );
			info = NEXT_LINEAR( info, struct shmem_info* );
		}


		
		// PHEW!!!!!!!!!! 

	if ( sh->internal != 0 )
			memory_free( sh->physical_pages, (void*)sh->physical_location );
	
	delete_rwlock( sh->lock );
	kfree( sh );
	return 0;
}




/** This will be called by the process cleanup procedures when
 * a process exists. Basically, it will scan the shared memory
 * list and release all shared memory regions which the
 * process had created and are not marked as immortal.
 *
 * \return 0 if success.
 */
int shmem_clean( int pid )
{
	int i = 0;
	struct shmem *sh;
	int id;

	//dmesg("%!clean_shmem: %i\n", pid );

	rwlock_get_read_access( shmem_lock );
	

	while ( (sh = htable_get( shmem_table, i++ ) ) != NULL )
	{
		if ( sh->pid != pid ) continue;
		

		id = sh->id;


		
			if ( (sh->flags & SHMEM_IMMORTAL) == SHMEM_IMMORTAL ) 
			{
				rwlock_get_write_access( sh->lock );		// Lock the region for writing.
					sh->flags = sh->flags | SHMEM_ORPHAN;	// It's an orphan :(
				rwlock_release( sh->lock );					// Release it.
				continue;	// Dn'We know it's pointless.
			}
			

			// Destroy this region!

			rwlock_release( shmem_lock );

				// Delete in a lockless state.
				int rc = shmem_delete( pid, id );
				if ( rc != 0 ) return -1;			
										/// \todo This is not a great situation. How do you deal with this?

				// Lock for re-entry. check.
			rwlock_get_read_access( shmem_lock );
			i = 0;	// reset index count, roger.
	}


	rwlock_release( shmem_lock );
	return 0;
}








