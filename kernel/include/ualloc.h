#ifndef _KERNEL_UALLOC_H
#define _KERNEL_UALLOC_H

#include <smk/inttypes.h>

#define UALLOC_NORELEASE		1

struct allocated_memory
{
	size_t pages;
	unsigned int flags;

	void*	phys;
	void* 	virt;

	int shared;
	
	struct process *parent;
	struct allocated_memory* next;
	struct allocated_memory* prev;
};




int user_alloc( struct process *proc, size_t pages, void** ptr );
int user_free( struct process *proc, void* ptr );
int user_size( struct process *proc, void* ptr, size_t *size );
int user_location( struct process *proc, void* ptr, void** physptr );
int user_map( struct process *proc, void *ptr, size_t pages, void **virtptr );
int user_ensure( struct process *proc, void* ptr, size_t pages );


int user_release_all( struct process *proc );

#endif

