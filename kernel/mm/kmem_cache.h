#ifndef _KMEM_CACHE_H
#define _KMEM_CACHE_H

#include <smk/limits.h>

#include <mm/mm.h>
#include "vmem.h"


#define	KMEM_NAME_LENGTH	SMK_NAME_LENGTH
#define KMEM_OBJECTS		1024


#ifndef _HAVE_KMEM_CTL_T
#define _HAVE_KMEM_CTL_T
typedef	unsigned int kmem_ctl_t;
#endif

#ifndef _HAVE_KMEM_REDZONE_T
#define _HAVE_KMEM_REDZONE_T
typedef	unsigned int kmem_redzone_t;
#define	RED_ZONE_DATA		0xdeadc007
#endif


#define	KV_NOSLEEP			0x00000001UL	// No wait for available memory.
#define	KV_NOGROW			0x00000002UL	// Don't extend the slab.
#define	KV_RED_ZONE			0x00000004UL	// Red zones before and after objects. (Catch buffer over/under runs)
#define	KV_OFF_SLAB			0x00000008UL	// Keep data structures off the slab.
#define KV_DEBUG_FREE		0x00000010UL	// Debug the frees of objects
#define KV_OWNERSHIP		0x00000020UL	// Keeps track of ownership per allocation




/** This is the kmem_slab. Objects are allocated in bulk using 
 * one allocation and slit up into objects. */
typedef struct kmem_slab
{
	kmem_ctl_t	position;	///< The position in the stack.

	size_t	offset;		///< The offset into the ptr for the first object.
	void*	ptr;		///< The pointer to a bulk allocation of objects.

	struct kmem_slab *next;	///< Linked list information.
	struct kmem_slab *prev;	///< Linked list information.
	
} kmem_slab_t;


/** This is the object cache structure proper which contains all
 * the information regarding the object cache. */
typedef struct kmem_cache
{
	char	name[KMEM_NAME_LENGTH];	///< The name of the kmem cache.
		
	size_t 	size;		///<  The size of an individual object
	size_t 	align;		///<  The alignment of the objects.
	size_t	objects;	///<  The number of objects per slab.

	size_t	int_size;	///<  The internal size of an object (including red zones, etc).
	size_t	int_slabobj_size;	///<  The actual size of object space in a slab
	
	void (*constructor)(void *obj, void *token, int kmflag);
	void (*destructor)(void *obj, void *token);
	void (*reclaim)(void *token);
	void* token;

	vmem_t*		vmem;		///< The arbitrary resource allocator;

	int 	cflags;		///<  Flags affecting the behaviour of the cache

	int 	total_objects;	///< The total num of objects owned by this cache
	int 	used_objects;	///< The total num of objects in use.

	kmem_slab_t  *slabs;	///< The linked list of slabs
	
} kmem_cache_t;




#ifdef  __cplusplus
extern "C" {
#endif

kmem_cache_t*	kmem_cache_create(
					const char *name, size_t size, size_t align,
					void (*constructor)(void *obj, void *token, int kmflag),
					void (*destructor)(void *obj, void *token),
					void (*reclaim)(void *token),
					void *token,
					vmem_t *vmem,
					int cflags);

void 	kmem_cache_destroy(kmem_cache_t *cp);

void*	kmem_cache_alloc(kmem_cache_t *cp, int kmflag);

void 	kmem_cache_free(kmem_cache_t *cp, void *obj);

size_t	kmem_cache_size(kmem_cache_t *cp);

const char*	kmem_cache_name(kmem_cache_t *cp);

size_t	kmem_cache_shrink(kmem_cache_t *cp);

kmem_cache_t*	kmem_cache_contains(kmem_cache_t *cp, void* obj);

#ifdef _TEST_
void kmem_cache_display( kmem_cache_t *cp );
#endif



#ifdef  __cplusplus
}
#endif


#endif

