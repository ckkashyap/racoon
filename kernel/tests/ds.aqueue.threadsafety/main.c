#include "include/stdlib.h"
#include "include/pthread.h"

#include "../../ds/aqueue.h"


#define THREADS			50
#define OBJECTS			1052

#define OBJECTSIZE		5
#define PAGEOBJECTS		10


struct ds_aqueue aq;		///< The testing aqueue
unsigned int	t_count;	///< The thread indication


void *thread_add_esafe( void *d )
{
	int i;
	int size = 0;

	aqueue_lock( &aq );

		size = aqueue_size( &aq );

			for ( i = 0; i < OBJECTS; i++ )
			{
				aqueue_new( &aq );
				aqueue_save( &aq );
			}

		if ( aqueue_size(&aq) != (size + OBJECTS) )
				exit( EXIT_FAILURE );

	aqueue_unlock( &aq );

	atomic_inc( &t_count );
}


void *thread_add_isafe( void *d )
{
	int i;

	for ( i = 0; i < OBJECTS; i++ )
	{
		aqueue_lock( &aq );

				aqueue_new( &aq );
				aqueue_save( &aq );
	
		aqueue_unlock( &aq );
	}

	atomic_inc( &t_count );
}

void *thread_rem_esafe( void *d )
{
	int i;
	int size;

	aqueue_lock( &aq );

		size = aqueue_size( &aq );

			for ( i = 0; i < OBJECTS; i++ )
			{
				aqueue_next( &aq );
			}

		if ( aqueue_size( &aq) != (size - OBJECTS) )
				exit( EXIT_FAILURE );
		
	aqueue_unlock( &aq );

	atomic_inc( &t_count );
}

void *thread_rem_isafe( void *d )
{
	int i;


	for ( i = 0; i < OBJECTS; i++ )
	{
		aqueue_lock( &aq );
		aqueue_next( &aq );
		aqueue_unlock( &aq );
	}


	atomic_inc( &t_count );
}





int main( int argc, char *argv )
{
	int i;
	pthread_t tid;

	aqueue_init( &aq, OBJECTSIZE , PAGEOBJECTS ); 

	// -------------------------
		t_count = 0;
		for ( i = 0; i < THREADS; i++ )
			if ( pthread_create( &tid, NULL, thread_add_esafe, NULL ) != 0 )
				return EXIT_FAILURE;

		while ( t_count != THREADS ) continue;

		if ( aqueue_size( &aq ) != (THREADS * OBJECTS) ) return EXIT_FAILURE;
		aqueue_reset( &aq );
	// -------------------------
				
		t_count = 0;
		for ( i = 0; i < THREADS; i++ )
			if ( pthread_create( &tid, NULL, thread_add_isafe, NULL ) != 0 )
				return EXIT_FAILURE;

		while ( t_count != THREADS ) continue;

		if ( aqueue_size( &aq ) != (THREADS * OBJECTS) ) return EXIT_FAILURE;
	// -------------------------


		t_count = 0;
		for ( i = 0; i < THREADS; i++ )
			if ( pthread_create( &tid, NULL, thread_rem_esafe, NULL ) != 0 )
				return EXIT_FAILURE;

		while ( t_count != THREADS ) continue;

		if ( aqueue_size( &aq ) != 0 ) return EXIT_FAILURE;
	// -------------------------

				
		t_count = 0;
		for ( i = 0; i < THREADS; i++ )
			if ( pthread_create( &tid, NULL, thread_add_isafe, NULL ) != 0 )
				return EXIT_FAILURE;

		while ( t_count != THREADS ) continue;

		if ( aqueue_size( &aq ) != (THREADS * OBJECTS) ) return EXIT_FAILURE;
	// -------------------------


		t_count = 0;
		for ( i = 0; i < THREADS; i++ )
			if ( pthread_create( &tid, NULL, thread_rem_isafe, NULL ) != 0 )
				return EXIT_FAILURE;

		while ( t_count != THREADS ) continue;

		if ( aqueue_size( &aq ) != 0 ) return EXIT_FAILURE;
	// -------------------------



		t_count = 0;
		for ( i = 0; i < THREADS; i++ )
		{
			if ( (i % 2) == 0 )
			{
				if ( pthread_create( &tid, NULL, thread_add_isafe, NULL ) != 0 )
					return EXIT_FAILURE;
			}
			else
			{
				if ( pthread_create( &tid, NULL, thread_rem_isafe, NULL ) != 0 )
					return EXIT_FAILURE;
			}
		}

		while ( t_count != THREADS ) continue;

		if (aqueue_size( &aq ) > ((THREADS * OBJECTS)/2) ) return EXIT_FAILURE;
	// -------------------------



	aqueue_reset( &aq );
	return EXIT_SUCCESS;
}





