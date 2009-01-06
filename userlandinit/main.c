#include <smk/sysenter.h>
#include <smk/strings.h>


void	dome( const char *name )
{
	struct syscall_packet scp;

	scp.opcode = OP( ENV, FIVE );
	kmemcpy( scp.name, name, SMK_NAME_LENGTH );

	_sysenter( &scp );
}


int main( int argc, char *argv[])
{
	while ( 1==1 )
	{
		dome( "heck heck heck" );
	}

	return 0;
}



