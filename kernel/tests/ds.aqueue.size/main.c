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


		// Add a lot
		for ( i = 0; i < 1052; i++ )
		{
			aqueue_new( &aq );
			aqueue_save( &aq );
		}

		// Are they all there?
		if ( aqueue_size( &aq ) != 1052 ) return EXIT_FAILURE;

		// Remove a lot of them
		for ( i = 0; i < 823; i++ )
		{
			void *tmp = aqueue_next( &aq );
			if ( tmp == NULL ) return EXIT_FAILURE;
		}

		// Is the remaining correct?
		if ( aqueue_size( &aq ) != (1052 - 823) ) return EXIT_FAILURE;

		// Add them back
		for ( i = 0; i < 823; i++ )
		{
			aqueue_new( &aq );
			aqueue_save( &aq );
		}

		// Are they all there?
		if ( aqueue_size( &aq ) != 1052 ) return EXIT_FAILURE;

		// Remove them again?
		for ( i = 0; i < 823; i++ )
		{
			void *tmp = aqueue_next( &aq );
			if ( tmp == NULL ) return EXIT_FAILURE;
		}

		// Is the remaining correct?
		if ( aqueue_size( &aq ) != (1052 - 823) ) return EXIT_FAILURE;

		// Remove the rest
		for ( i = 0; i < (1052-823); i++ )
		{
			void *tmp = aqueue_next( &aq );
			if ( tmp == NULL ) return EXIT_FAILURE;
		}

		// Is it empty?
		if ( aqueue_size( &aq ) != 0 ) return EXIT_FAILURE;

		// Ensure that there's nothing left.
		for ( i = 0; i < 1000; i++ )
		{
			if ( aqueue_next( &aq ) != NULL ) return EXIT_FAILURE;
		}
			

	aqueue_unlock( &aq );

	aqueue_reset( &aq );
	return EXIT_SUCCESS;
}





