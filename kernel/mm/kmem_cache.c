#include <smk/strings.h>

#include <debug/assert.h>
#include <mm/mm.h>

#include "kmem_cache.h"

#define ALLOC( vmem, size, flags ) kmalloc( size )
#define FREE( vmem, ptr, size ) kfree( ptr )



static kmem_ctl_t* get_ctl_array( kmem_slab_t *slab )
{
	assert( slab != NULL );
	return ((kmem_ctl_t*)((kmem_slab_t*)slab + 1));
}


static void* get_object_ptr( kmem_cache_t* cp, kmem_slab_t *slab, size_t index )
{
	assert( (cp != NULL) && (slab != NULL) && (index < cp->objects) );
	return (slab->ptr + slab->offset + index * cp->int_size);
}


static size_t get_object_index( kmem_cache_t* cp, kmem_slab_t *slab, void *obj )
{
	assert( (cp != NULL) && (slab != NULL) && (obj != NULL) );
	assert( (((size_t)(obj - slab->ptr - slab->offset)) % cp->int_size) == 0);
	return ((size_t)(obj - slab->ptr - slab->offset))/cp->int_size;
}


static kmem_slab_t* get_object_slab( kmem_cache_t* cp, void* obj )
{
	assert( (cp != NULL) && (obj != NULL) );
	kmem_slab_t *tmp = cp->slabs;
	while ( tmp != NULL )
	{
		if ( (tmp->ptr <= obj) && (obj < (tmp->ptr + cp->int_slabobj_size)) )
				return tmp;

		tmp = tmp->next;
	}
	
	return NULL;
}

#define REDZONE_LEFT		0
#define REDZONE_RIGHT		1

static kmem_redzone_t* get_object_redzone( kmem_cache_t* cp, void *obj, int side )
{
	assert( (cp != NULL) && (obj != NULL) && ((side == REDZONE_LEFT) || (side == REDZONE_RIGHT)) );
	assert( (cp->cflags & KV_RED_ZONE) == KV_RED_ZONE );

	if ( side == REDZONE_LEFT ) return (((kmem_redzone_t*)obj) - 1);
	return ((kmem_redzone_t*)( obj + cp->size ));
}


static kmem_cache_t** get_object_owner_ptr( kmem_cache_t* cp, void *obj )
{
	assert( (cp != NULL) && (obj != NULL) );
	assert( (cp->cflags & KV_OWNERSHIP) == KV_OWNERSHIP );

	void *tmp = obj;
	if ( (cp->cflags & KV_RED_ZONE) == KV_RED_ZONE ) 
		tmp = get_object_redzone( cp, tmp, REDZONE_LEFT );

	return (((kmem_cache_t**)tmp) - 1);
}






// ---- internal functions --------------

static kmem_slab_t*	kmem_new_slab( kmem_cache_t* cp, int kmflag )
{
	assert( cp != NULL );

	kmem_slab_t *slab = NULL;

	size_t man_space = sizeof(kmem_slab_t) + cp->objects * sizeof(kmem_ctl_t);

	if ( (kmflag & KV_OFF_SLAB) == KV_OFF_SLAB )
	{
		slab = (kmem_slab_t*) ALLOC( cp->vmem, man_space, kmflag );
		if ( slab == NULL ) return NULL;

		slab->ptr = ALLOC( cp->vmem, cp->int_slabobj_size, kmflag );
		if ( slab->ptr == NULL )
		{
			FREE( cp->vmem, slab, man_space );
			return NULL;
		}
	}
	else
	{
		slab = (kmem_slab_t*) ALLOC( cp->vmem, man_space + cp->int_slabobj_size, kmflag );
		if ( slab == NULL ) return NULL;

		slab->ptr = ((kmem_ctl_t*)(slab + 1)) + cp->objects;
	}


	slab->offset = 0;

	if ( (cp->cflags & KV_RED_ZONE) == KV_RED_ZONE )
			slab->offset += sizeof( kmem_redzone_t );

	if ( (cp->cflags & KV_OWNERSHIP) == KV_OWNERSHIP )
			slab->offset += sizeof( kmem_cache_t** );

	if ( (cp->align > 1) && (((size_t)(slab->ptr + slab->offset) % cp->align) != 0) )
		slab->offset += cp->align - (((size_t)(slab->ptr + slab->offset)) % cp->align);

	slab->next	= NULL;
	slab->prev	= NULL;
	return slab;
}






static void kmem_init_slab( kmem_cache_t* cp, kmem_slab_t *slab, int kmflag )
{
	assert( (cp != NULL) && (slab != NULL) );

	int i;
	kmem_ctl_t *ctl = get_ctl_array( slab );

	for ( i = 0; i < cp->objects; i++ )
	{
		void *obj = get_object_ptr( cp, slab, i );

		if ( cp->constructor != NULL )
			cp->constructor( obj, cp->token, kmflag );


		if ( (cp->cflags & KV_RED_ZONE) == KV_RED_ZONE )
		{
			*(get_object_redzone( cp, obj, REDZONE_LEFT )) = RED_ZONE_DATA;
			*(get_object_redzone( cp, obj, REDZONE_RIGHT )) = RED_ZONE_DATA;
		}

		if ( (cp->cflags & KV_OWNERSHIP) == KV_OWNERSHIP )
		{
			*(get_object_owner_ptr( cp, obj )) = cp;
		}
	

		ctl[i] = i;
	}


	cp->total_objects += cp->objects;

	slab->position = 0;
}


static void	kmem_destroy_slab( kmem_cache_t* cp, kmem_slab_t *slab )
{
	assert( (cp != NULL) && (slab != NULL) );

	size_t man_space = sizeof(kmem_slab_t) + cp->objects * sizeof(kmem_ctl_t);

	if ( cp->destructor != NULL )
	{
		int i;

		for ( i = 0; i < cp->objects; i++ )
			cp->destructor( get_object_ptr( cp, slab, i ), cp->token );
	}


	if ( (cp->cflags & KV_OFF_SLAB) == KV_OFF_SLAB )
	{
		FREE( cp->vmem, slab->ptr, cp->int_slabobj_size );
		FREE( cp->vmem, slab, man_space );
	}
	else
	{
		FREE( cp->vmem, slab, man_space + cp->int_slabobj_size );
	}

	cp->total_objects -= cp->objects;
}





// --------------------------------------


kmem_cache_t*	kmem_cache_create(
					const char *name,
					size_t size,
					size_t align,
					void (*constructor)(void *obj, void *token, int kmflag),
					void (*destructor)(void *obj, void *token),
					void (*reclaim)(void *token),
					void *token,
					vmem_t *vmem,
					int cflags)
{
	kmem_cache_t *kmem = (kmem_cache_t*)ALLOC( vmem, sizeof(kmem_cache_t), cflags );
	if ( kmem == NULL ) return NULL;

		if ( name != NULL )
			kstrncpy( kmem->name, name, KMEM_NAME_LENGTH );
		else
			kmem->name[0] = 0;

		kmem->size 	= size;
		kmem->align = align;
		kmem->objects = KMEM_OBJECTS;
		kmem->constructor = constructor;
		kmem->destructor = destructor;
		kmem->reclaim = reclaim;
		kmem->token = token;
		kmem->vmem = vmem;
		kmem->cflags = cflags;

		kmem->total_objects = 0;
		kmem->used_objects = 0;

		kmem->slabs = NULL;


		// Calculate internal size of objects
		kmem->int_size = size;

		if ( (cflags & KV_RED_ZONE) == KV_RED_ZONE )
			kmem->int_size += 2 * sizeof( kmem_redzone_t );

		if ( (cflags & KV_OWNERSHIP) == KV_OWNERSHIP )
			kmem->int_size += sizeof( kmem_cache_t** );

		if ( (kmem->align > 1) && ((kmem->int_size % kmem->align) != 0) )
				kmem->int_size += (kmem->align - ( kmem->int_size %  kmem->align ));


		// Calculate slab object space size
#warning This is a guess.
		kmem->int_slabobj_size = (kmem->int_size * kmem->objects + kmem->align);

		if ( (kmem->cflags & KV_RED_ZONE) == KV_RED_ZONE )
				kmem->int_slabobj_size += sizeof( kmem_redzone_t );


	return kmem;
}


void 	kmem_cache_destroy(kmem_cache_t *cp)
{
	assert( cp != NULL );
	kmem_slab_t*	slab = cp->slabs;

	while ( slab != NULL )
	{
		kmem_slab_t *tmp_slab = slab->next;
		kmem_destroy_slab( cp, slab );
		slab = tmp_slab;
	}

	FREE( cp->vmem, cp, sizeof(kmem_cache_t) );
}



static kmem_slab_t* kmem_cache_grow( kmem_cache_t *cp, int kmflag )
{
	assert( cp != NULL );

	kmem_slab_t *slab = kmem_new_slab( cp, kmflag );
	if ( slab == NULL ) return NULL;

	kmem_init_slab( cp, slab, kmflag );

	slab->next = cp->slabs;
	if ( cp->slabs != NULL ) cp->slabs->prev = slab;
	cp->slabs = slab;

	return slab;
}


void*	kmem_cache_alloc(kmem_cache_t *cp, int kmflag)
{
	assert( cp != NULL );

	// Take the first available object
	if ( (cp->slabs != NULL) && (cp->slabs->position != cp->objects))
	{
		kmem_ctl_t *ctl = get_ctl_array( cp->slabs );
		cp->used_objects += 1;
		return get_object_ptr( cp, cp->slabs, ctl[ (cp->slabs->position)++ ] );
	}


	// Ensure not empty
	if ( cp->total_objects == cp->used_objects )
	{
		if ( (cp->cflags & KV_NOGROW) == KV_NOGROW ) return NULL;
		if ( kmem_cache_grow( cp, kmflag ) == NULL ) 
		{
			if ( cp->reclaim == NULL ) return NULL;
			cp->reclaim( cp->token );
			if ( kmem_cache_grow( cp, kmflag ) == NULL ) return NULL;
		}

		return kmem_cache_alloc( cp, kmflag );
	}

	
	// Search for some pages which have objects
	{
		kmem_slab_t *slab = cp->slabs->next;
		assert( slab != NULL );
	
		while ( slab->position == cp->objects )
		{
			slab = slab->next;
			assert( slab != NULL );
		}
	
		assert( slab != cp->slabs );
		assert( slab != NULL );
	
	
		if ( slab->next != NULL ) slab->next->prev = slab->prev;
		slab->prev->next = slab->next;
		cp->slabs->prev = slab;
		slab->next = cp->slabs;
		slab->prev = NULL;
		cp->slabs = slab;
	}

	return kmem_cache_alloc( cp, kmflag );
}


void 	kmem_cache_free(kmem_cache_t *cp, void *obj)
{
	if ( (cp == NULL) || (obj == NULL) ) return;

	kmem_slab_t *slab = get_object_slab( cp, obj );
	if ( slab == NULL ) return;

	if ( (cp->cflags & KV_RED_ZONE) == KV_RED_ZONE )
	{
		assert( *(get_object_redzone( cp, obj, REDZONE_LEFT )) == RED_ZONE_DATA );
		assert( *(get_object_redzone( cp, obj, REDZONE_RIGHT )) == RED_ZONE_DATA );
	}

	kmem_ctl_t *ctl = get_ctl_array( slab );
	assert( ctl != NULL );

	ctl[ --(slab->position) ] = get_object_index( cp, slab, obj );
	cp->used_objects -= 1;
}


size_t	kmem_cache_size(kmem_cache_t *cp)
{
	assert( (cp != NULL) );
	return cp->size;
}

const char*	kmem_cache_name(kmem_cache_t *cp)
{
	assert( (cp != NULL) );
	return cp->name;
}

size_t	kmem_cache_shrink(kmem_cache_t *cp)
{
	assert( (cp != NULL) );
	kmem_slab_t *slab = cp->slabs;

	// Search for some pages which have objects
	while ( slab != NULL )
	{
		kmem_slab_t *next = slab->next;

		if ( slab->position == 0 )
		{
			if ( next != NULL ) next->prev = slab->prev;
			if ( slab->prev != NULL ) slab->prev->next = next;
			if ( slab == cp->slabs ) cp->slabs = next;


			kmem_destroy_slab( cp, slab );
		}

		slab = next;
	}


	return 0;
}



kmem_cache_t*	kmem_cache_contains(kmem_cache_t *cp, void* obj)
{
	assert( (cp != NULL) && (obj != NULL) );
	if ( (cp->cflags & KV_OWNERSHIP) == KV_OWNERSHIP )
	{
		if ( *(get_object_owner_ptr( cp, obj )) == cp ) 
			return cp;
		else 
			return NULL;
	}

	if ( get_object_slab( cp, obj ) != NULL ) return cp;
	return NULL;
}



#ifdef _TEST_
void kmem_cache_display( kmem_cache_t *cp )
{
	kmem_slab_t *slab;

	printf( "kmem_cache_t: %s (%u of %u used) (flags = %u)\n", 
				cp->name, cp->used_objects, cp->total_objects, cp->cflags );
	printf( "\tvmem object: %p\n", cp->vmem );
	printf( "\t%u objects of %u bytes (real size %u) per slab, aligned to %u\n",
					cp->objects, cp->size, cp->int_size, cp->align );

	printf( "\thooks (constructor = %p, destructor = %p, reclaim = %p, token = %p)\n",
					cp->constructor, cp->destructor, cp->reclaim, cp->token );


	slab = cp->slabs;
	while ( slab != NULL )
	{
		printf( "\t[ slab = %p, ctl = %p -> %p, ptr = %p, offset = %x, off_ptr = %p ] position = %u (free = %u)\n",
				slab, 
				get_ctl_array( slab ),
				get_ctl_array( slab ) + cp->objects,
				slab->ptr,
				slab->offset,
				slab->ptr + slab->offset,
				slab->position,
				cp->objects - slab->position );

		slab = slab->next;
	}

}
#endif

