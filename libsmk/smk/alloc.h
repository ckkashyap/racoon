#ifndef _LIBKERNEL_ALLOC_H
#define _LIBKERNEL_ALLOC_H

/**  \defgroup ALLOC Slab Allocator operations
 *
 * libsmk makes use of liballoc and the memory operations 
 * provided by it. This is to ensure independence from 
 * any other standard libraries and provide a base to all
 * applications based on it.
 * 
 */

/** @{ */

void* smk_malloc(int size);
void  smk_free(void *ptr);
void* smk_calloc(int nobj, int size);
void* smk_realloc(void *p, int size);

/** @} */


#endif

