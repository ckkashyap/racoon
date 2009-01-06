#include <smk/inttypes.h>
#include <smk/err.h>
#include <processes/process.h>
#include "include/paging.h"
#include <mm/mm.h>
#include "include/ualloc.h"

#include <processes/threads.h> // sk_BASE + CEILING
#include "mm/physmem.h"


int user_alloc( struct process *proc, size_t pages, void** ptr )
{
   struct allocated_memory *mem;
   void *location;
   uint32_t found;

   if ( pages == 0 ) return SMK_REQUESTED_NOTHING;

	location = memory_alloc( pages );
	if ( location == NULL ) return SMK_NOMEM;


	// Allocate this here to make failure returns easier later on.
    mem = kmalloc( sizeof( struct allocated_memory ) );
	if ( mem == NULL )
	{
		memory_free( pages, location );
		return SMK_NOMEM;
	}

	found = page_provide( proc->map,
						  (unsigned int) location,
						  sK_BASE,
						  sK_CEILING,
						  pages,
						  USER_PAGE | READ_WRITE );
	
	if ( found == 0 )
	{
		memory_free( pages, location );
		kfree( mem );
		return SMK_NOMEM;
	}

    mem->pages = pages;
    mem->flags = 0;
    mem->shared = 0;
    mem->phys = location;
    mem->virt = (void*)found;
    mem->prev = NULL;
    mem->next = proc->alloc;
    mem->parent = proc;
    proc->alloc = mem;
    if ( mem->next != NULL ) mem->next->prev = mem;

	*ptr = (void*)location;

   return SMK_OK;
}


static unsigned int user_alloc_hard( struct process *proc, void* virtual, size_t pages )
{
   struct allocated_memory *mem;
   void *location;
   uint32_t found;
   
   assert( pages != 0 );

    location = memory_alloc( pages );
	if ( location == NULL ) return SMK_NOMEM;

	// Allocate this here to make failure returns easier later on.
    mem = kmalloc( sizeof( struct allocated_memory ) );
	if ( mem == NULL )
	{
		memory_free( pages, location );
		return SMK_NOMEM;
	}
   

	// A problem occurs here where ensure will ensure virtual regions.
	// However, since this is ualloc, we're keeping track of the
	// regions. This means that ensure may not overlap in this function.
	found = page_ensure( proc->map, (uint32_t)virtual, pages, USER_PAGE | READ_WRITE );
	
	if ( found < 0 )
	{
		memory_free( pages, location );
		kfree( mem );
		return SMK_NOMEM;
	}

	assert( found == 0 );	// We need to be 100% sure that we're not sharing.
	
    mem->pages = pages;
    mem->flags = 0;
    mem->shared = 0;
    mem->phys = location;
    mem->virt = (void*)found;
    mem->prev = NULL;
    mem->next = proc->alloc;
    mem->parent = proc;
    proc->alloc = mem;
    if ( mem->next != NULL ) mem->next->prev = mem;

   
   return SMK_OK;
}


int user_free( struct process *proc, void* ptr )
{
	struct allocated_memory *mem;
	int release_mem;
	
	
	  mem = proc->alloc;
	  while ( mem != NULL )
	  {
  	    if ( mem->virt == ptr ) break;
	    mem = mem->next;
	  }

	  if ( mem == NULL ) return SMK_NOT_FOUND;

// remove mem from the process list.
	  if ( proc->alloc == mem ) proc->alloc = mem->next;
	  if ( mem->next != NULL ) mem->next->prev = mem->prev;
	  if ( mem->prev != NULL ) mem->prev->next = mem->next;

// orphan it
	  mem->parent = NULL;
	  
	release_mem = 1;

// Is it shared??
	  // yes => we can't release physical
	  if ( mem->shared > 0 ) release_mem = 0;
// Is it safe to release?
	  if ( (mem->flags & UALLOC_NORELEASE) != 0 ) release_mem = 0;

// Unmap it

		 if ( release_mem == 1 ) memory_free( mem->pages, mem->phys );
	
		 page_release( proc->map, (uint32_t)(mem->virt), mem->pages );	

	 if ( release_mem == 1 ) kfree( mem );
	return SMK_OK;
}


/*
 *
 * user_size
 * 
 * Returns the size of an allocated region as identified
 * by pointer
 *
 */

int user_size( struct process *proc, void* ptr, size_t *size )
{
	struct allocated_memory *mem = proc->alloc;
	
	  while ( mem != NULL )
	  {
  	    if ( mem->virt == ptr) break;
	    mem = mem->next;
	  }

	  if ( mem == NULL ) return SMK_NOT_FOUND;

		*size = (mem->pages * PAGE_SIZE);

	return SMK_OK;
}

/*
 *
 * user_location
 * 
 * Returns the physical location of an allocated region as identified
 * by pointer
 *
 */

int user_location( struct process *proc, void* ptr, void** physptr )
{
	struct allocated_memory *mem = proc->alloc;
	
	  while ( mem != NULL )
	  {
  	    if ( mem->virt == ptr ) break;
	    mem = mem->next;
	  }

	  if ( mem == NULL ) return SMK_NOT_FOUND;
	  *physptr = mem->phys;
	  return SMK_OK;
}


/*
 * Maps in a specified section of physical memory into userspace on request.
 */

int user_map( struct process *proc, void *ptr, size_t pages, void **virtptr )
{
   struct allocated_memory *mem;
   uint32_t found;

   if ( pages == 0 ) return SMK_REQUESTED_NOTHING;

   if ( ((uint32_t)ptr & (PAGE_SIZE-1)) != 0 ) return SMK_NOT_ALIGNED;
   
	found = page_provide( proc->map,
						  (uint32_t)ptr,
						  sK_BASE,
						  sK_CEILING,
						  pages,
						  USER_PAGE | READ_WRITE );
	
    if ( found != 0 )
    {
       mem = kmalloc( sizeof( struct allocated_memory ) );
       mem->pages = pages;
       mem->flags = UALLOC_NORELEASE;
       mem->shared = 0;
       mem->phys = ptr;
       mem->virt = (void*)found;
       mem->prev = NULL;
       mem->next = proc->alloc;
       mem->parent = proc;
       proc->alloc = mem;
       if ( mem->next != NULL ) mem->next->prev = mem;
    }

	*virtptr = (void*)found;
   
   return SMK_OK;
}


int user_release_all( struct process *proc )
{
	while ( proc->alloc != NULL )
		user_free( proc, proc->alloc->virt );

	assert( proc->alloc == NULL );
	return 0;
}



/** This function has been implemented to correctly and cleanly
 * cut up the virtual destination so that they don't overlap
 * with each other.
 * 
 * The point of this is so that we can now make use of 
 * ualloc functions when mapping in process code. This 
 * makes management of the memory space a heck(!) of a lot
 * easier for me. Plus it provides one interface for me.
 *
 * \note Please note that this is not a fast function. It's just
 * used in the process set up so that we shouldn't really have
 * any problems anyway..
 *
 * \todo Anyway to improve this?
 */ 

int user_ensure( struct process *proc, void* ptr, size_t pages )
{
	uint32_t index;
	uint32_t end = (uint32_t)ptr + pages * PAGE_SIZE;
	uint32_t last_start = (uint32_t)ptr;
	
	// Do a sequential search for open blocks and ualloc them.
	
	for ( index = (uint32_t)ptr; index < end; index += PAGE_SIZE )
	{
		if ( page_present( proc->map, index ) != 0 ) continue;
			
		if ( index != last_start )
		{
			int rc = user_alloc_hard( proc, (void*)last_start, (index-last_start) / PAGE_SIZE );
			if ( rc != SMK_OK ) return rc;
		}

		last_start = index + PAGE_SIZE;
	}


	if ( last_start < end )
	{
		int rc = user_alloc_hard( proc, (void*)last_start, (end - last_start) / PAGE_SIZE );
		if ( rc != SMK_OK ) return rc;
	}

	return SMK_OK;
}







