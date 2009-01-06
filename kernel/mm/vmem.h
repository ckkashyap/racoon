#ifndef _VMEM_H
#define _VMEM_H


#include <smk/inttypes.h>
#include <smk/limits.h>


#define VMEM_VERSION			0.90
#define	VMEM_NAME_LENGTH		SMK_NAME_LENGTH



#define	VMEM_BESTFIT		1U
#define VMEM_INSTANT		2U
#define	VMEM_SEARCH			4U


#define	VMEM_SLEEP			0x000U
#define	VMEM_NOSLEEP		0x100U


#define VMEM_FREE			0
#define VMEM_ALLOCATED		1



/** The boundary tag used to identify ranges */
typedef struct vmem_bt
{
	void* 	addr;			///<  The first valid unit
	size_t 	size;			///<  The size of this range
	void*	source_addr;	///<  The source address returned
	size_t  allocated;		///<  Indication of allocation state

	struct vmem_bt *next;	///<  Linked list information
	struct vmem_bt *prev;	///<  Linked list information

	struct vmem_bt *fl_next;	///< Free-list next;
	struct vmem_bt *fl_prev;	///< Free-list prev;
} vmem_bt_t;


typedef struct vmem_span
{
	void* 	 	addr;
	size_t 		size;
	size_t		slots;

	vmem_bt_t	**hash;		///<  The allocated hash table.
	
	struct vmem_span *next;
} vmem_span_t;



/** The actual structure used to maintain the resource
 * allocator */
typedef struct vmem
{
	char 		name[VMEM_NAME_LENGTH];	///< The name of the vmem.
	size_t 		quantum;				///< The units of the resources
	size_t	 	cache_max;				///< The maximum range size to cache
	size_t		cache_bits;				///< The cache bits
	void* 		(*afunc)(struct vmem*, size_t, int);
	void* 		(*ffunc)(struct vmem*, void*, int);
	struct vmem*	source;		


	vmem_bt_t 	*tags;				///< The ordered, llist of boundary tags
	vmem_span_t	*spans;				///< The ordered, llist of spans
	vmem_bt_t	**free_cache;		///< The free page caches

	int			vmflag;

} vmem_t;



#ifdef  __cplusplus
extern "C" {
#endif


vmem_t*		vmem_create( const char* name,
						void* base,
						size_t size,
						size_t quantum,
						void* (*afunc)(vmem_t *, size_t, int),
						void* (*ffunc)(vmem_t *, void*, int),
						vmem_t* source,
						size_t qcache_max,
						int vmflag );

void	vmem_destroy( vmem_t* vmem );

void*	vmem_alloc( vmem_t* vmem, size_t size, int vmflag );
void* 	vmem_free( vmem_t* vmem, void *addr, size_t size );

void*	vmem_xalloc( vmem_t *vmem, size_t size, size_t align, 
					size_t phase, size_t nocross, 
					void *minaddr, void *maxaddr, int vmflag );
void 	vmem_xfree( vmem_t *vmem, void *addr, size_t size );

void*	vmem_add( vmem_t *vmem, void *addr, size_t size, int vmflag );


#ifdef  __cplusplus
}
#endif


#endif

