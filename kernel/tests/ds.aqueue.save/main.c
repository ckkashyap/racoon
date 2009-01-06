#include "include/stdlib.h"

#include "../../ds/aqueue.h"


#define OBJECTSIZE		5
#define PAGEOBJECTS		10

int main( int argc, char *argv )
{
	struct ds_aqueue aq;
	int i;

	aqueue_init( &aq, OBJECTSIZE , PAGEOBJECTS ); 
	aqueue_lock( &aq );


		for ( i = 0; i < 100; i++ )
		{
			aqueue_save( &aq );
			aqueue_next( &aq );
		}


		for ( i = 0; i < 100; i++ )
			aqueue_save( &aq );

		for ( i = 0; i < 100; i++ )
			aqueue_next( &aq );


	aqueue_unlock( &aq );
	aqueue_reset( &aq );
	return EXIT_SUCCESS;
}





