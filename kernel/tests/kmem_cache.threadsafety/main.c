#include "include/stdlib.h"
#include "include/stdio.h"
#include <debug/assert.h>

#include "include/pthread.h"


#include "../../arb/kmem_cache.h"


#define REQUIREMENT		20000
#define THREADS			50


static int total_constructed 	= 0;
static unsigned int t_count 	= THREADS;
static kmem_cache_t *kmem		= NULL;


struct doit_st { int num; };




int doit_constructor( void *obj, void *private, int kmflags )
{
	((struct doit_st*)obj)->num = 5;
	total_constructed += 1;
	return 0;
}


void doit_destructor( void *obj, void *private )
{
	if ( ((struct doit_st*)obj)->num != 5 ) exit( EXIT_FAILURE );
	total_constructed -= 1;
}


void* my_thread( void* data )
{
	int i;
	int pos = 0;
	struct doit_st *stack[REQUIREMENT]; 

		for ( i = 0; i < REQUIREMENT; i++ )
		{
			stack[pos++] = kmem_cache_alloc( kmem, 0 );
			if ( stack[pos-1]->num != 5 ) exit( EXIT_FAILURE );
		}


		for ( i = 0; i < REQUIREMENT; i++ )
		{
			kmem_cache_free( kmem, stack[--pos] );
		}


	atomic_dec( &t_count);
	return NULL;
}


int main( int argc, char *argv[] )
{
	int i;
	pthread_t tid;


	kmem = kmem_cache_create( sizeof(struct doit_st), 0,
													0,
													doit_constructor,
													doit_destructor,
													(void*)0xABCDE,
													malloc,
													free,
													66 );


		for ( i = 0; i < THREADS; i++ )
			if ( pthread_create( &tid, NULL, my_thread, NULL ) != 0 )
				return EXIT_FAILURE;

		while ( t_count != 0 ) continue;



	kmem_cache_destroy( kmem );
	if ( total_constructed != 0 ) return EXIT_FAILURE;

	exit( EXIT_SUCCESS );
}


