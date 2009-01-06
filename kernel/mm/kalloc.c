#include <smk/inttypes.h>
#include <smk/strings.h>
#include <mm/mm.h>

#include <debug/assert.h>


#include "kmem_cache.h"


#define ALIGNMENT		4
#define CACHE_UNIT		32
#define MAX_CACHES		12



static kmem_cache_t* caches[ MAX_CACHES ];


static void reclaim(void *token)
{
	int i;

	for ( i = 0; i < MAX_CACHES; i++ )
		kmem_cache_shrink( caches[i] );
}


static int get_cache_location( void *ptr )
{
	int i;

	for ( i = 0; i < MAX_CACHES; i++ )
		if ( kmem_cache_contains( caches[i], ptr ) == caches[i] ) 
			return i;

	return -1;
}


static int get_cache_num( size_t size )
{
	int i;
	size_t unit;

	for ( i = 0, unit = CACHE_UNIT; i < MAX_CACHES; i++, unit *= 2 )
		if ( size <= unit ) return i;

	return -1;
}


static void* l_kmalloc(size_t size)
{
	assert( size > 0 );
	int num = get_cache_num( size );
	assert( num >= 0 );
	return kmem_cache_alloc( caches[num], 0 );
}

static void l_kfree(void *ptr)
{
	assert( ptr != NULL );
	int	num = get_cache_location( ptr );
	assert( num >= 0 );
	kmem_cache_free( caches[num], ptr );
}

static void* l_krealloc(void *ptr, size_t size)
{
	if ( ptr == NULL ) return l_kmalloc( size );
	if ( size == 0 ) 
	{
		l_kfree( ptr );
		return NULL;
	}

	int num = get_cache_location( ptr );
	assert( num >= 0 );
		
	size_t prevsize = kmem_cache_size( caches[num] );

	if ( prevsize < size )
	{
		void *new_ptr = l_kmalloc( size );
		kmemcpy( new_ptr, ptr, prevsize );
		l_kfree( ptr );
		return new_ptr;
	}

	return ptr;
}





void init_kalloc()
{
	int i;
	unsigned int unit;

	dmesg( "Creating kalloc caches.\n" );

	for ( i = 0, unit = CACHE_UNIT; i < MAX_CACHES; i++, unit *= 2 )
	{

		dmesg( "    kalloc_cache( %i, %i )\n", unit, ALIGNMENT );
		caches[i] = kmem_cache_create( "kalloc_cache",
										unit,
										ALIGNMENT,
										NULL,
										NULL,
										reclaim,
										NULL,
										NULL,
										0 );

		assert( caches[i] != NULL );
	}




	struct mm_subsystem sub;

		sub.malloc = l_kmalloc;
		sub.free = l_kfree;
		sub.realloc = l_krealloc;

	set_mm_system( sub );
}




