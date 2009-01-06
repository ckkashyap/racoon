#include "include/stdlib.h"

#include "../../arb/arb.h"

#define LINEAR		100
#define RANDOM		250


int check_ranges( arb_t *arb, int num )
{
	int i = 0;
	int rc = 0;


	arbnum_t last_start = 0;
	arbnum_t last_size = 0;

	while ( rc >= 0 )
	{
		arbnum_t start;
		arbnum_t size;
		
		rc = arb_get( arb, i, &start, &size );
		if ( rc < 0 ) break;

		if ( rc != ARB_FREE ) return -1;			// All free
		if ( start < (last_start + last_size) ) return -1;	// All in order
						

		last_start = start;
		last_size = size;


		i++;
	}

	if ( i != num ) return -1;
	return 0;
}

int add_linear_up( int unit_size )
{
	int i = 0;
	arbnum_t pos = 0;

	arb_t *arb = arb_create( unit_size, malloc, free, 5 * unit_size );
	if ( arb == NULL ) return -1;
	
		for ( i = 0; i < LINEAR; i++ )
		{
			arbnum_t size = ((random() % 10)+1) * unit_size;
			if ( arb_add_range( arb, pos, size ) != 0 ) return -1;

			pos = pos + size;
			pos = pos + (random() % 10) * unit_size;
		}

	i = check_ranges( arb, LINEAR );

	arb_destroy( arb );
	return i;
}

int add_linear_down( int unit_size )
{
	int i = 0;
	arbnum_t pos = 0xF0000000;

	arb_t *arb = arb_create( unit_size, malloc, free, 5 * unit_size );
	if ( arb == NULL ) return -1;
	
		for ( i = 0; i < LINEAR; i++ )
		{
			arbnum_t size = ((random() % 10)+1) * unit_size;
			if ( arb_add_range( arb, pos-size, size ) != 0 ) return -1;

			pos = pos - size;
			pos = pos - (random() % 10) * unit_size;
		}

	i = check_ranges( arb, LINEAR );

	arb_destroy( arb );
	return i;
}



int main( int argc, char *argv )
{
	int i;
		
	for ( i = 1; i < 2000; i++ )
	{
		if ( add_linear_up( i * 0x10 ) != 0 ) return EXIT_FAILURE;
		if ( add_linear_down( i * 0x10 ) != 0 ) return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}





