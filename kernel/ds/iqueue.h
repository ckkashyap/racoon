#ifndef _KERNEL_DS_IQUEUE_H
#define _KERNEL_DS_IQUEUE_H


#include <smk/inttypes.h>

#define IQUEUE_NOGROW		0x1
#define IQUEUE_NOSHRINK		0x2

struct iqueue
{
	size_t	initial;
	size_t	grow;
	size_t	size;
	size_t	start;
	size_t	count;
	int *queue;
};


struct iqueue* iqueue_create( size_t initial_size, size_t grow_size );
void iqueue_destroy( struct iqueue* iq );

int iqueue_add( struct iqueue*, int num, int iqflags );
int iqueue_next( struct iqueue*, int iqflags );

int iqueue_count( struct iqueue* );


#endif

