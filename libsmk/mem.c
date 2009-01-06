#include <smk/inttypes.h>
#include <smk/err.h>
#include <smk/syscalls.h>
#include <smk/sysenter.h>


int smk_mem_alloc(  size_t pages,  void **ptr  )
{
	struct syscall_packet scp;

	if ( pages == 0 ) return SMK_BAD_PARAMS;
	if ( ptr == NULL ) return SMK_BAD_PARAMS;
	scp.size = pages;
	scp.opcode = OP( MEMORY, ONE );

	_sysenter( &scp );

	if ( scp.rc != SMK_OK ) return scp.rc;
	*ptr = scp.ptr;
	return scp.rc;
}



int smk_mem_free(  void* ptr  )
{
	struct syscall_packet scp;

	if ( ptr == NULL ) return SMK_BAD_PARAMS;
	scp.ptr = ptr;
	scp.opcode = OP( MEMORY, TWO );

	_sysenter( &scp );

	return scp.rc;
}



int smk_mem_size(  void* ptr,  ssize_t *size  )
{
	struct syscall_packet scp;

	if ( ptr == NULL ) return SMK_BAD_PARAMS;
	if ( size == NULL ) return SMK_BAD_PARAMS;
	scp.ptr = ptr;
	scp.opcode = OP( MEMORY, THREE );

	_sysenter( &scp );

	if ( scp.rc != SMK_OK ) return scp.rc;
	*size = scp.size;
	return scp.rc;
}



int smk_mem_location(  void* ptr,  void **location  )
{
	struct syscall_packet scp;

	if ( ptr == NULL ) return SMK_BAD_PARAMS;
	if ( location == NULL ) return SMK_BAD_PARAMS;
	scp.ptr = ptr;
	scp.opcode = OP( MEMORY, FOUR );

	_sysenter( &scp );

	if ( scp.rc != SMK_OK ) return scp.rc;
	*location = scp.ptr;
	return scp.rc;
}



int smk_mem_map(  void* location,  size_t pages,  void **ptr  )
{
	struct syscall_packet scp;

	if ( location == NULL ) return SMK_BAD_PARAMS;
	if ( pages == 0 ) return SMK_BAD_PARAMS;
	if ( ptr == NULL ) return SMK_BAD_PARAMS;
	scp.ptr = location;
	scp.size = pages;
	scp.opcode = OP( MEMORY, FIVE );

	_sysenter( &scp );

	if ( scp.rc != SMK_OK ) return scp.rc;
	*ptr = scp.ptr;
	return scp.rc;
}

