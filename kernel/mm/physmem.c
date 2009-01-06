#include <smk/inttypes.h>
#include <smk/atomic.h>
#include <debug/assert.h>
#include "physmem.h"



volatile static BLOCK_TYPE *memory_bitmap = NULL;
volatile static size_t	 memory_pages;
volatile static void*	 memory_offset;

static int s_freePages = 0;		///< The total number of free pages.
static int s_lastPage = 0;  	///< The last page used in the bitmap.

spinlock_t kernel_lock = INIT_SPINLOCK;


#define ISSET(page)	\
	((memory_bitmap[(page)/BLOCK_BITS ] & (BLOCK_TYPE)(1ul<<((page) % BLOCK_BITS))) != 0)

#define SET(page)		\
	atomic_bitset( &(memory_bitmap[ (page)/BLOCK_BITS ]), (page) % BLOCK_BITS )

#define UNSET(page)		\
	atomic_bitclear( &(memory_bitmap[ (page)/BLOCK_BITS ]), (page) % BLOCK_BITS )


static void* memory_multiple_alloc( size_t pages )
{
	int i, j;
	size_t block;
	void *ptr;

	for ( i = 0; i < memory_pages; i++ )
	{
		int badone = 0;

		block = (i + s_lastPage) % memory_pages;
		//dmesg( "Checking block %i (%i out of %i)\n", block, i, memory_pages );

		if ( (block + pages) > memory_pages )  // Ignore wrapper-around
		{
			i += pages - 2;
			continue;	
		}

				// Scan from the current pos up for pages amount.
		for ( j = 0; j < pages; j++ )
		{
			if ( ISSET( block + j ) ) 
			{
				i = i + j;
				badone = 1;
				break;
			}
		}

		if ( badone != 1 ) break; 
	}

	if ( i >= memory_pages ) return NULL;
	
	//dmesg( "Found %i (%i out of %i)\n", block, i, memory_pages );

	for ( i = 0; i < pages; i++ )
	{
		SET( block + i );
		s_freePages -= 1;
	}

	s_lastPage = block + pages;

	ptr = (void*)(((uintptr_t)block) * PAGE_SIZE + (uintptr_t)memory_offset);
	return ptr;
}



void* memory_alloc( size_t pages )
{
	void *ptr = NULL;
	assert( pages > 0 );

	acquire_spinlock( &kernel_lock );

		if ( s_freePages > pages )
			ptr = memory_multiple_alloc( pages );
		else
			ptr = NULL;

	release_spinlock( &kernel_lock );
	return ptr;
}



void memory_free( size_t pages, void *position )
{
	size_t start, i;
	uintptr_t pos = (uintptr_t)(position - memory_offset);

	assert( (pos % PAGE_SIZE) == 0 );
	start = pos / PAGE_SIZE;

	acquire_spinlock( &kernel_lock );

		for (i = 0; i < pages; i++)
		{
			int tmp = UNSET( start+i );
			assert( tmp == 1  );
			s_freePages += 1;
		}
	
	release_spinlock( &kernel_lock );
}


void memory_set( void *pstart, void *pend, int used )
{
	size_t i, start, pages;

	if ( used == 1 )
	{
		if ( (((uintptr_t)pstart) % PAGE_SIZE) != 0 )
			pstart = (void*)(((uintptr_t)pstart / PAGE_SIZE) * PAGE_SIZE);

		if ( (((uintptr_t)pend) % PAGE_SIZE) != 0 )
			pend = (void*)((((uintptr_t)pend / PAGE_SIZE)+1) * PAGE_SIZE);
	}
	else
	{
		if ( (((uintptr_t)pstart) % PAGE_SIZE) != 0 )
			pstart = (void*)((((uintptr_t)pstart / PAGE_SIZE)+1) * PAGE_SIZE);

		if ( (((uintptr_t)pend) % PAGE_SIZE) != 0 )
			pend = (void*)(((uintptr_t)pend / PAGE_SIZE) * PAGE_SIZE);
	}


	start = (uintptr_t)(pstart - memory_offset) / PAGE_SIZE;
	pages = (uintptr_t)(pend - pstart) / PAGE_SIZE;


	acquire_spinlock( &kernel_lock );

		for (i = 0; i < pages; i++)
		{
			if ( used == 1 )
			{
				int tmp = SET( start+i );
				if ( tmp == 0 ) s_freePages -= 1;
			}
			else
			{
				int tmp = UNSET( start+i );
				if ( tmp == 1 ) s_freePages += 1;
			}
		}
	
	release_spinlock( &kernel_lock );
}



#define	TO_PTR( page )	(void*)(((uintptr_t)page) * PAGE_SIZE + (uintptr_t)memory_offset)

void show_memory_map()
{
	int i = 0;
	int last = 0;
	int taken = 0;
	if ( ISSET(0) ) taken = 1;
	
	dmesg("Physical memory summary (%i total pages, %i free pages):\n", memory_pages, s_freePages);

	for ( i = 0; i < memory_pages; i++ )
	{
		if ( ISSET(i) && (taken == 0) )
		{
			dmesg("  Available: %p  %p\n", TO_PTR( last ), TO_PTR( i ) );
			last = i;
			taken = 1;
		}
		else
		if ( (! (ISSET(i))) && ( taken == 1 ) )
		{
			dmesg("  Unavailable: %p  %p\n", TO_PTR( last ), TO_PTR( i ) );
			last = i;
			taken = 0;
		}
	}

	if ( taken == 0 )
		dmesg("  Available: %x  %x\n", TO_PTR( last ), TO_PTR( memory_pages ) );
	else
		dmesg("  Unavailable: %x  %x\n", TO_PTR( last ), TO_PTR( memory_pages ) );
}





void init_physmem( void* offset, uint32_t *map, size_t pages )
{
	int i,j;

	assert( (s_freePages == 0) && (s_lastPage == 0) );
	assert( ((BLOCK_TYPE)-1) > 0 );	// Check signed nature of type

	memory_offset = offset;
	memory_bitmap = map;
	memory_pages = pages;

	j = pages / BLOCK_BITS;
	for ( i = 0; i < j; i++ )
		memory_bitmap[i] = (BLOCK_TYPE)-1;

	j = j * BLOCK_BITS;
	for ( i = 0; i < (pages % BLOCK_BITS); i++ )
		SET( j + i );

	assert( (s_freePages == 0) && (s_lastPage == 0) );
}




