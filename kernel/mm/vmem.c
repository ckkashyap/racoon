#include <debug/assert.h>
#include "vmem.h"

#include <smk/strings.h>
#include <mm/mm.h>



static void* vm_afunc(vmem_t *vmem, size_t size, int vmflag )
{
	return vmem_alloc( vmem, size, vmflag );
}

static void* vm_ffunc(vmem_t *vmem, void* ptr, int vmflag)
{
	return vmem_free( vmem, ptr, vmflag );
}

// -------------------------------------------------
static inline vmem_span_t	*vmem_getspan( vmem_t *vmem, void* res )
{
	vmem_span_t *span = vmem->spans;

	while ( span != NULL )
	{
		if ( (res >= span->addr) && (res < (span->addr + span->size)) )
			break;

		span = span->next;
	}

	return span;
}


/** Returns an integer(i) such that:  2^(i+1) > value >= 2^i  */
static inline int highbit( uintptr_t value )
{
	int i;

	asm ( "movl $-1, %%eax;"
		  "bsrl %1, %%eax;"
		  "movl %%eax, %0;"
		  : "=g" (i)
		  : "g" (value)
		  : "eax" );

	assert( i >= 0 );
	return i;
}

static inline int cache_num( vmem_t* vmem, size_t size )
{
	int pos = highbit( size/vmem->quantum );
	if ( pos >= vmem->cache_bits ) return -1;
	return pos;
}


/** Adds the bt to the freelists */
static inline void 	add_freelist( vmem_t *vmem, vmem_bt_t *bt )
{
	int pos = highbit( bt->size/vmem->quantum );
	assert( pos >= 0 );
	assert( bt->fl_next == NULL );
	assert( bt->fl_prev == NULL );

	if ( vmem->free_cache[pos] != NULL )
			vmem->free_cache[pos]->fl_prev = bt;

	bt->fl_prev = NULL;
	bt->fl_next = vmem->free_cache[pos];
	vmem->free_cache[pos] = bt;
}

/** remove a bt from a freelist */
static inline void 	rem_freelist( vmem_t *vmem, vmem_bt_t *bt )
{
	int pos = cache_num( vmem, bt->size );
	assert( pos >= 0 );

	if ( vmem->free_cache[pos] == bt )
			vmem->free_cache[pos] = bt->fl_next;

	if ( bt->fl_prev != NULL ) bt->fl_prev->fl_next = bt->fl_next;
	if ( bt->fl_next != NULL ) bt->fl_next->fl_prev = bt->fl_prev;

	bt->fl_next = NULL;
	bt->fl_prev = NULL;
}

// -------------------------------------------------



vmem_t*		vmem_create( const char* name,
						void* base,
						size_t size,
						size_t quantum,
						void* (*afunc)(vmem_t *, size_t, int),
						void* (*ffunc)(vmem_t *, void*, int),
						vmem_t* source,
						size_t qcache_max,
						int vmflag )
{
	vmem_t *vmem = kmalloc( sizeof( vmem_t ) );
	if ( vmem == NULL ) return NULL;


		kstrncpy( vmem->name, name, VMEM_NAME_LENGTH );

		vmem->quantum = quantum;
		vmem->afunc = (afunc == NULL) ? vm_afunc : afunc;
		vmem->ffunc = (ffunc == NULL) ? vm_ffunc : ffunc;
		vmem->source = source;
		vmem->vmflag = vmflag;
		vmem->tags 	= NULL;
		vmem->spans = NULL;

		vmem->cache_max = qcache_max;
		vmem->cache_bits = highbit(qcache_max/quantum) + 1;
		vmem->free_cache = NULL;

		if ( vmem->cache_bits > 0 )
		{
			int i = 0;

			vmem->free_cache = kmalloc(sizeof(vmem_bt_t*) * vmem->cache_bits);
			if ( vmem->free_cache == NULL )
			{
				kfree( vmem );
				return NULL;
			}

			for ( i = 0; i < vmem->cache_bits; i++ )
				vmem->free_cache[i] = NULL;
		}

	if ( size > 0 )
		if ( vmem_add( vmem, base, size, vmflag ) != base )
		{
			vmem_destroy( vmem );
			return NULL;
		}

	return vmem;
}




void		vmem_destroy( vmem_t* vmem )
{
	vmem_span_t *span = vmem->spans;
	vmem_bt_t *bt = vmem->tags;

	while ( bt != NULL )
	{
		vmem_bt_t *tmp = bt->next;
		kfree( bt );
		bt = tmp;
	}

	while ( span != NULL )
	{
		vmem_span_t *tmp = span->next;
		kfree( span->hash );
		kfree( span );
		span = tmp;
	}

	if ( vmem->free_cache != NULL )
		kfree( vmem->free_cache );

	kfree( vmem );
}


/** This splits a boundary tag into two boundary tags (required size
 * is on the right) and returns a pointer to the boundary tag
 * of requested size. 
 *
 * The new tag of requested size is marked as VMEM_FREE but not 
 * in the freelist.
 *
 */ 
static vmem_bt_t*	vmem_al_splittag( vmem_t *vmem, vmem_bt_t *bt, size_t size )
{
	vmem_bt_t *tmp = kmalloc( sizeof( vmem_bt_t ) );
	if ( tmp == NULL ) return NULL;

	assert( bt->allocated == VMEM_FREE );
			
	// truncate bt
		bt->size 		= bt->size - size;

	// temporary
		tmp->addr 		= bt->addr + bt->size;
		tmp->size		= size;
		tmp->source_addr = NULL;
		tmp->prev		= bt;
		tmp->next		= bt->next;
		tmp->fl_next	= NULL;
		tmp->fl_prev	= NULL;
		tmp->allocated 	= VMEM_FREE;

		if ( tmp->next != NULL )
				tmp->next->prev = tmp;

		bt->next		= tmp;

	return tmp;
}


static vmem_bt_t* vmem_alloc_search( vmem_t* vmem, size_t rsize, int vmflag )
{
	vmem_bt_t *bt = vmem->tags;
	vmem_bt_t *best_bt = NULL;
		
	while ( bt != NULL )
	{
		if ( (bt->allocated == VMEM_FREE) && (bt->size >= rsize) ) 
		{
			if ( (vmflag & VMEM_BESTFIT) != VMEM_BESTFIT ) return bt;
			if ( (best_bt == NULL) || (bt->size < best_bt->size) )
				best_bt = bt;
		}
		bt = bt->next;
	}

	return best_bt;
}

static vmem_bt_t* vmem_alloc_quantum( vmem_t* vmem, size_t rsize, int vmflag )
{
	vmem_bt_t *best_bt = NULL;

	int pos = highbit( rsize/vmem->quantum );
	if ( (vmflag & VMEM_INSTANT) == VMEM_INSTANT ) pos += 1; // instant fit.

	while ( pos < vmem->cache_bits )
	{
		vmem_bt_t *bt = vmem->free_cache[ pos++ ];
		while ( bt != NULL )
		{
			assert( bt->allocated == VMEM_FREE );

			if ( bt->size >= rsize ) 
			{
				if ( (vmflag & VMEM_BESTFIT) != VMEM_BESTFIT ) return bt;
				if ( (best_bt == NULL) || (bt->size < best_bt->size) )
					best_bt = bt;
			}
			bt = bt->fl_next;
		}

		if ( ((vmflag & VMEM_BESTFIT) == VMEM_BESTFIT) && (best_bt != NULL)) 
			break;
	}
		
	return best_bt;
}


void*	vmem_alloc( vmem_t* vmem, size_t rsize, int vmflag )
{
	vmem_bt_t *bt = NULL;
	size_t size = rsize;

	if ( rsize == 0 ) return NULL;

	if ( (rsize % vmem->quantum) != 0 ) 
			size += (vmem->quantum - (rsize % vmem->quantum));

	if ( (vmflag & VMEM_SEARCH) != VMEM_SEARCH )
	{
		bt = vmem_alloc_quantum( vmem, size, vmflag );
		if ( bt == NULL ) 
		{
			bt = vmem_alloc_search( vmem, size, vmflag );
		}
	}
	else
		bt = vmem_alloc_search( vmem, size, vmflag );

	if ( bt == NULL ) return NULL;

	assert( bt->allocated == VMEM_FREE );

	if ( cache_num( vmem, bt->size ) >= 0 )
			rem_freelist( vmem, bt );

	if ( (bt->size - size) >= vmem->quantum )	// We can split this tag
	{
		vmem_bt_t* tmp = vmem_al_splittag( vmem, bt, size );
		if ( cache_num( vmem, bt->size ) >= 0 )
				add_freelist( vmem, bt );
		bt = tmp;
	}

	bt->allocated = VMEM_ALLOCATED;

	vmem_span_t *span = vmem_getspan( vmem, bt->addr );
	assert( span != NULL );

	assert( span->hash[ (bt->addr - span->addr) / vmem->quantum ] == NULL );
	span->hash[ (bt->addr - span->addr) / vmem->quantum ] = bt; 

	if ( vmem->source != NULL ) 
	{
		bt->source_addr = vmem->afunc( vmem->source, bt->size, vmflag );
		if ( bt->source_addr == NULL )	// oops
		{
			vmem_free( vmem, bt->addr, rsize );
			return NULL;
		}
	}

	//dmesg( "vmem_alloc( %p, %i, %i ) -> ( %i, %p, %p )\n",
	//			vmem, rsize, vmflag,
	//			bt->size, bt->addr, bt->source_addr );

	return bt->addr;
}


void* 	vmem_free( vmem_t* vmem, void *addr, size_t size )
{
	vmem_bt_t *tmp;

	vmem_span_t *span = vmem_getspan( vmem, addr );
	if ( span == NULL ) return NULL;

	vmem_bt_t *bt = span->hash[ (addr - span->addr) / vmem->quantum ]; 
	if ( bt == NULL ) return NULL;

	if ( (vmem->source != NULL) && (bt->source_addr != NULL) ) 
		vmem->ffunc( vmem->source, bt->source_addr, bt->size );


	assert(  span->hash[ (addr - span->addr) / vmem->quantum ]  != NULL );
	span->hash[ (addr - span->addr) / vmem->quantum ] = NULL; 
				// Clear the BT

	assert( bt->allocated == VMEM_ALLOCATED );

	bt->allocated = VMEM_FREE; 

	// Now we melt it back in.

	if ( bt->prev != NULL )	// Left
	{
		if ((bt->prev->allocated == VMEM_FREE) && (bt->prev->addr >= span->addr))
		{
			if ( cache_num( vmem, bt->prev->size ) >= 0 )
				rem_freelist( vmem, bt->prev );
				
			bt->prev->size  = bt->prev->size + bt->size;
			bt->prev->next 	= bt->next;

			if ( bt->next != NULL )
					bt->next->prev = bt->prev;

			tmp = bt->prev;

			kfree( bt );
			bt = tmp;
		}
	}

	// bt is still valid

	if ( bt->next != NULL )
	{
		if ( (bt->next->allocated == VMEM_FREE) && 
					(bt->next->addr < (span->addr + span->size))  )	// Right
		{
			if ( cache_num( vmem, bt->next->size ) >= 0 )
				rem_freelist( vmem, bt->next );
				
			bt->size = bt->size + bt->next->size;

			tmp = bt->next;

			bt->next = tmp->next;

			if ( tmp->next != NULL )
					tmp->next->prev = bt;

			kfree( tmp );
		}
	}

	// bt is still valid

	if ( cache_num( vmem, bt->size ) >= 0 )
		add_freelist( vmem, bt );	// Insert new size into free list
	
	return addr;
}



void*	vmem_xalloc( vmem_t *vmem, 
					size_t size, 
					size_t align, 
					size_t phase, 
					size_t nocross, 
					void *minaddr, 
					void *maxaddr, 
					int vmflag )
{
	assert( (vmem == NULL) && (vmem != NULL) );
	assert( "this is not implemented" == "true" );
	return NULL;
}

void 	vmem_xfree( vmem_t *vmem, void *addr, size_t size )
{
	assert( (vmem == NULL) && (vmem != NULL) );
	assert( "this is not implemented" == "true" );
}






void*	vmem_add( vmem_t *vmem, void *addr, size_t size, int vmflag )
{
	assert( size > 0 );
	assert( (size % vmem->quantum) == 0 );
	assert( (addr + size) > (addr) );

	vmem_span_t *insert_span = NULL;
	vmem_span_t *span = vmem->spans;
	while ( span != NULL )
	{
		if ( span->addr >= addr ) break;

		insert_span = span;
		span = span->next;
	}

	// Create the structure
	span = kmalloc( sizeof( vmem_span_t ) );
	if ( span == NULL ) return NULL;


		span->addr = addr;
		span->size = size;
		span->slots = (size / (vmem->quantum));

		span->hash = kmalloc( span->slots * sizeof(vmem_bt_t*) );
		if ( span->hash == NULL )
		{
			kfree( span );
			return NULL;
		}


	kmemset( span->hash, 0, span->slots * sizeof(vmem_bt_t*) );

		
	/* ------------------------------------------------------- */
	vmem_bt_t *insert = NULL;
	vmem_bt_t *bt = vmem->tags;

	while ( bt != NULL )
	{
		if ( bt->addr >= addr ) break;

		insert = bt;
		bt = bt->next;
	}

	// Create the BT structure
	bt = kmalloc( sizeof( vmem_bt_t ) );
	if ( bt == NULL ) 
	{
		kfree( span->hash );
		kfree( span );
		return NULL;
	}

		// ------ SPAN INSERT --------------
		// Insert with disregard to overlapping.
		if ( insert_span == NULL )
		{
			span->next = vmem->spans;
			vmem->spans = span;
		}
		else
		{
			span->next = insert_span->next;
			insert_span->next = span;
		}
		// ------ END OF SPAN INSERT
	

		bt->addr = addr;
		bt->size = size;
		bt->allocated = VMEM_FREE;
		bt->fl_next = NULL;
		bt->fl_prev = NULL;

		// Insert with disregard to overlapping.
		if ( insert == NULL )
		{
			bt->next = vmem->tags;
			bt->prev = NULL;
			vmem->tags = bt;
		}
		else
		{
			bt->next = insert->next;
			bt->prev = insert;
		}
	
	
		if ( bt->prev != NULL ) bt->prev->next = bt;
		if ( bt->next != NULL ) bt->next->prev = bt;

	if ( cache_num( vmem, bt->size ) >= 0 )
		add_freelist( vmem, bt );
	return addr;
}





