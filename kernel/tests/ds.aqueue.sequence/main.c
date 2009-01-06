#include "include/stdlib.h"

#include "../../ds/aqueue.h"


#define PAGES		50

#define OBJECTSIZE		200
#define PAGEOBJECTS		1024

int main( int argc, char *argv )
{
	struct ds_aqueue aq;
	uintptr_t  ptr1, ptr2;
	uintptr_t  page_ptr1, page_ptr2;
	int i,j;

	aqueue_init( &aq, OBJECTSIZE , PAGEOBJECTS ); 

		aqueue_lock( &aq );

		page_ptr1 = 0;
		
		for ( j = 0; j < PAGES; j++ )
		{
			for ( i = 0; i < (PAGEOBJECTS/2); i++ )
			{
				ptr1 = (uintptr_t)aqueue_new( &aq );
				aqueue_save( &aq );

				ptr2 = (uintptr_t)aqueue_new( &aq );
				aqueue_save( &aq );


				if ( i == 0 ) page_ptr2 = ptr1;

				if ( ptr2 != (ptr1 + OBJECTSIZE) )
					return EXIT_FAILURE;
			}

			if ( (page_ptr2 < (page_ptr1 + OBJECTSIZE*PAGEOBJECTS)) &&
				 (page_ptr2 >= page_ptr1) )
					return EXIT_FAILURE;

			page_ptr1 = page_ptr2;
		}
			

		aqueue_unlock( &aq );

	aqueue_reset( &aq );
	return EXIT_SUCCESS;
}





