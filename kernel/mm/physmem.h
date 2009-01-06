#ifndef _KERNEL_PHYSMEM_H
#define _KERNEL_PHYSMEM_H

#include <smk/inttypes.h>
#include "multiboot.h"


#define BLOCK_TYPE	unsigned int
#define	BLOCK_SIZE  sizeof(BLOCK_TYPE)
#define	BLOCK_BITS	(BLOCK_SIZE*8)



void init_physmem( void* offset, uint32_t *map, size_t pages );
void show_memory_map();

void* memory_alloc( size_t pages );
void memory_free( size_t pages, void *position );

void memory_set( void *start, void *end, int used );


#endif
