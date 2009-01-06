#ifndef _MM_MM_H
#define _MM_MM_H

#include <smk/inttypes.h>


typedef void* (*malloc_t)(size_t);
typedef void (*free_t)(void*);
typedef void* (*realloc_t)(void*,size_t);


struct mm_subsystem
{
	malloc_t malloc;
	free_t free;
	realloc_t realloc;
};



void set_mm_system( struct mm_subsystem sub );


void* kmalloc(size_t);
void kfree(void*);
void* krealloc(void*,size_t);


#endif

