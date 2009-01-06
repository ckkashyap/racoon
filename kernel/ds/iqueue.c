#include <mm/mm.h>
#include <debug/assert.h>
#include "iqueue.h"



struct iqueue* iqueue_create( size_t initial_size, size_t grow_size )
{
	struct iqueue* iq = (struct iqueue*)kmalloc( sizeof(struct iqueue) );
	if ( iq == NULL ) return NULL;

	iq->initial = initial_size;
	iq->grow = grow_size;
	iq->size = initial_size;
	iq->start = 0;
	iq->count = 0;

	if ( initial_size > 0 )
	{
		iq->queue = (int*)kmalloc(sizeof(int)*initial_size);
		if ( iq->queue == NULL )
		{
			kfree( iq );
			return NULL;
		}
	}
	else
		iq->queue = NULL;

	return iq;
}

void iqueue_destroy( struct iqueue* iq )
{
	assert( iq != NULL );
	if ( iq->queue != NULL ) kfree( iq->queue );
	kfree( iq );
}


int iqueue_add( struct iqueue* iq, int num, int iqflags )
{
	assert( iq != NULL );

	if ( iq->size == iq->count )
	{
		if ( (iq->grow == 0) || ((iqflags & IQUEUE_NOGROW) == IQUEUE_NOGROW ))
				return -1;

		int* new_queue = (int*)kmalloc( (iq->size + iq->grow) * sizeof(int) );
		if ( new_queue == NULL ) return -1;
		else
		{
			int i;
			for ( i = 0; i < iq->count; i++ )
				new_queue[i] = iq->queue[ (iq->start + i) % iq->size ];

			iq->start = 0;
			iq->size += iq->grow;
			kfree( iq->queue );
			iq->queue = new_queue;
		}
	}

	iq->queue[ (iq->start + iq->count++) % iq->size ] = num;
	return 0;
}

int iqueue_next( struct iqueue* iq, int iqflags )
{
	assert( iq != NULL );
	assert( iq->count > 0 );

	int rc = iq->queue[iq->start];
	iq->start = (iq->start + 1) % iq->size;
	iq->count -= 1;

	if ( (iq->grow != 0) && ((iqflags & IQUEUE_NOSHRINK) != IQUEUE_NOSHRINK ))
	{
		if ( (iq->size > iq->initial) && ((iq->count % iq->grow) == 0) )
		{
			int i;
			int* new_queue = (int*)kmalloc( (iq->size - iq->grow) * sizeof(int) );
			if ( new_queue == NULL ) return rc;

			for ( i = 0; i < iq->count; i++ )
				new_queue[i] = iq->queue[ (iq->start + i) % iq->size ];

			iq->start = 0;
			iq->size -= iq->grow;
			kfree( iq->queue );
			iq->queue = new_queue;
		}
	}
	return rc;
}


int iqueue_count( struct iqueue* iq )
{
	assert( iq != NULL );
	return iq->count;
}



