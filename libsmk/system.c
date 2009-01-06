#include <smk/inttypes.h>
#include <smk/err.h>
#include <smk/syscalls.h>
#include <smk/sysenter.h>
#include <smk/systemdefs.h>


int smk_system_info(  struct system_info *info  )
{
	struct syscall_packet scp;

	if ( info == NULL ) return SMK_BAD_PARAMS;
	scp.ptr = info;
	scp.opcode = OP( SYSTEM, ONE );

	_sysenter( &scp );

	return scp.rc;
}



int smk_process_info(  int pid,  struct process_info *info  )
{
	struct syscall_packet scp;

	if ( (pid <= 0) || (pid >= SMK_MAX_PID) ) return SMK_NOT_FOUND;
	if ( info == NULL ) return SMK_BAD_PARAMS;
	scp.ptr = info;
	scp.pid = pid;
	scp.opcode = OP( SYSTEM, TWO );

	_sysenter( &scp );

	return scp.rc;
}

