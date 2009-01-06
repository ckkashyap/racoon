#include <ds/aqueue.h>
#include <smk/strings.h>
#include <debug/assert.h>


/**  \addtogroup DS_AQUEUE
 *
 */

/** @{ */


/** Initializes a queue for use.  
 *
 * \param aq A pointer to the aqueue structure
 * \param obj_size  The size of an individual object in the queue
 * \param obj_count The number of objects to keep per page
 */
struct aqueue* aqueue_create( size_t size, size_t count )
{
	struct aqueue* aq = (struct aqueue*)kmalloc( sizeof(struct aqueue) );
	if ( aq == NULL ) return NULL;

	atomic_init( &(aq->lock) );

	aq->pages 		= 0;
	aq->obj_size	= size;
	aq->obj_count	= count;

	aq->position		= 0;
	aq->last_position 	= 0;

	aq->page 		= NULL;
	aq->last_page	= NULL;

	return aq;
}

void aqueue_destroy( struct aqueue* aq )
{
	struct aqueue_page *tmp = NULL;

	while ( aq->page != NULL )
	{
		tmp = aq->page;
		aq->page = tmp->next;
		kfree( tmp );
	}

	kfree( aq );
}



int aqueue_add( struct aqueue *aq, void* obj )
{
	struct aqueue_page *page = NULL;
	void* position;

	acquire_spinlock( &(aq->lock) );
		
	int page_div = aq->last_position / aq->obj_count;
	int page_mod = aq->last_position % aq->obj_count;
	

	if ( (page_mod == 0) && (aq->pages <= page_div) )
	{
		size_t size = sizeof(struct aqueue_page) + aq->obj_count * aq->obj_size;
		page = (struct aqueue_page*)kmalloc( size );
		if ( page == NULL ) 
		{
			release_spinlock( &(aq->lock) );
			return -1;
		}

		page->next 	= NULL;
		aq->pages 	+= 1;

		if ( aq->page == NULL ) aq->page = page;
						else	aq->last_page->next = page;

		aq->last_page = page;
	}

	page = aq->last_page;

	position = page;
	position = position + sizeof( struct aqueue_page ) + aq->obj_size * page_mod;
	kmemcpy( position, obj, aq->obj_size );
	release_spinlock( &(aq->lock) );
	return 0;
}



int aqueue_next( struct aqueue *aq, void* obj )
{
	void* position;

	acquire_spinlock( &(aq->lock) );

	int page_div = aq->position / aq->obj_count;
	int page_mod = aq->position % aq->obj_count;

	assert( aq->position != aq->last_position );


	while ( page_div > 0 )
	{
		struct aqueue_page *tmp = aq->page;
		aq->page = aq->page->next;
		kfree( tmp );
		assert( aq->page != NULL );

		aq->last_position 	-= aq->obj_count;
		aq->position 		-= aq->obj_count;
		aq->pages -= 1;

		page_div -= 1;	// Down one.
	}


	position = aq->page;
	position += sizeof( struct aqueue_page ) + aq->obj_size * page_mod;

	aq->position += 1;

	kmemcpy( obj, position, aq->obj_size );
	release_spinlock( &(aq->lock) );
	return 0;
}


/** Returns the number of objects in the queue */
size_t aqueue_count( struct aqueue *aq )
{
	size_t rc;
	acquire_spinlock( &(aq->lock) );
	rc = (aq->last_position - aq->position);
	release_spinlock( &(aq->lock) );
	return rc;
}




/** @} */


