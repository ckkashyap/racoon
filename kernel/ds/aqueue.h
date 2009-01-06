#ifndef _KERNEL_DS_AQUEUE_H
#define _KERNEL_DS_AQUEUE_H

#include <smk/inttypes.h>
#include <smk/atomic.h>
#include <mm/mm.h>

/**  \defgroup DS_AQUEUE  Atomic FIFO queue of arbitrarily sized objects
 *
 */

/** @{ */


#define	AQUEUE_NOGROW		0x1
#define AQUEUE_NOSHRINK		0x2


/** This is an individual page in the queue. It contains the objects. 
 * The last page in a queue will always have next set to NULL. It's
 * a linked-list.
 */
struct aqueue_page
{
	struct aqueue_page *next;	///< The next page.
};

/** This is the primary aqueue structure which contains
 * information about the queue and the pages within it.
 * It's initialized by the aqueue_init() call. It maintains
 * a linked-list of pages. This structure can be cleaned
 * by calling aqueue_reset();
 */
struct aqueue
{
	spinlock_t 	 lock;			///< The lock for this queue
	volatile unsigned int pages;	///< The number of active pages in queue
	size_t obj_size;		///< The size of an object in the queue
	size_t obj_count;		///< The number of objects per page

	volatile size_t position;		///< Position in the page for reading.
	volatile size_t last_position; ///< Position of the first free slot.

	struct aqueue_page *page;		///< The first page in a queue.
	struct aqueue_page *last_page;	///< The last page in a queue.
};

/** @} */

struct aqueue*	aqueue_create( size_t size, size_t count );

void	aqueue_destroy( struct aqueue* aq );

int 	aqueue_add( struct aqueue *aq, void* obj );
int 	aqueue_next( struct aqueue *aq, void* obj );

size_t 	aqueue_count( struct aqueue *aq );


#endif

