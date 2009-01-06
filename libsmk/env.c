#include <smk/inttypes.h>
#include <smk/err.h>
#include <smk/syscalls.h>
#include <smk/sysenter.h>


int smk_setenv(  const char name[SMK_ENVNAME_LENGTH],  void *data,  size_t size  )
{
	struct syscall_packet scp;

	if ( (data == NULL) && (size > 0) ) return SMK_BAD_PARAMS;
	kmemcpy( scp.name, name, sizeof(scp.name) );
	scp.ptr = data;
	scp.size = size;
	scp.opcode = OP( ENV, ONE );

	_sysenter( &scp );

	return scp.rc;
}



ssize_t smk_getenv(  const char name[SMK_ENVNAME_LENGTH],  void *data,  size_t size  )
{
	struct syscall_packet scp;

	if ( (data == NULL) ) return SMK_BAD_PARAMS;
	kmemcpy( scp.name, name, sizeof(scp.name) );
	scp.ptr = data;
	scp.size = size;
	scp.opcode = OP( ENV, TWO );

	_sysenter( &scp );

	if ( scp.rc != SMK_OK ) return scp.rc;
	return scp.size;
}



ssize_t smk_getenv_size(  const char name[SMK_ENVNAME_LENGTH]  )
{
	struct syscall_packet scp;

	kmemcpy( scp.name, name, sizeof(scp.name) );
	scp.opcode = OP( ENV, THREE );

	_sysenter( &scp );

	if ( scp.rc != SMK_OK ) return scp.rc;
	return scp.size;
}



int smk_getenv_info(  int i,  const char name[SMK_ENVNAME_LENGTH],  size_t *size  )
{
	struct syscall_packet scp;

	scp.id = i;
	scp.opcode = OP( ENV, FOUR );

	_sysenter( &scp );

	return scp.rc;
}



int smk_clearenv(  const char name[SMK_ENVNAME_LENGTH]  )
{
	struct syscall_packet scp;

	kmemcpy( scp.name, name, sizeof(scp.name) );
	scp.opcode = OP( ENV, FIVE );

	_sysenter( &scp );

	return scp.rc;
}

