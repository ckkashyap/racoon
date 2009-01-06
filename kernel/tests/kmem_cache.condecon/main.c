#include "include/stdlib.h"
#include "include/stdio.h"
#include <debug/assert.h>

#include "../../arb/kmem_cache.h"


#define REQUIREMENT		200000


static int total_constructed = 0;

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



int main( int argc, char *argv[] )
{
	int i;
	int pos = 0;
	struct doit_st *stack[REQUIREMENT]; 

	kmem_cache_t*	kmem = kmem_cache_create( sizeof(struct doit_st), 0,
													0,
													doit_constructor,
													doit_destructor,
													(void*)0xABCDE,
													kmalloc,
													kfree,
													66 );
	assert( kmem != NULL );

		
		for ( i = 0; i < REQUIREMENT; i++ )
		{
			stack[pos++] = kmem_cache_alloc( kmem, 0 );
			if ( stack[pos-1]->num != 5 ) exit( EXIT_FAILURE );
		}

		for ( i = 0; i < REQUIREMENT; i++ )
		{
			kmem_cache_free( kmem, stack[--pos] );
		}



	kmem_cache_destroy( kmem );
	if ( total_constructed != 0 ) return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}


