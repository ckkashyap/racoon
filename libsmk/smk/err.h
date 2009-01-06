#ifndef _LIBKERNEL_ERR_H
#define _LIBKERNEL_ERR_H


#define SMK_OK						0
#define SMK_ERROR					-1
#define SMK_NOMEM					-2
#define SMK_PID_NOT_FOUND			-3
#define SMK_TID_NOT_FOUND			-4
#define SMK_RESOURCE_ALLOCATED		-5
#define SMK_RESOURCE_BUSY			-6
#define SMK_TIMEOUT					-7
#define SMK_BAD_PARAMS				-8
#define SMK_NOT_OWNER				-9
#define SMK_UNKNOWN_SYSCALL			-10
#define SMK_UNSAFE_SYSCALL_INT		-11
#define SMK_SPAWN_FAILED			-12
#define SMK_SYSCALL_ERROR			-13
#define SMK_NOT_FOUND				-14
#define SMK_REQUESTED_NOTHING		-15
#define SMK_NOT_ALIGNED				-16

#define SMK_MAXERR					16

/** These are error strings */
static const char* smk_errorstrings[] = 
{
	"OK",
	"general error",
	"out of memory",
	"PID not found",
	"TID not found",
	"Requested resource is already allocated",
	"Resource busy",
	"Timeout",
	"Bad parameters were given",
	"Not the owner",
	"Unknown syscall",
	"Unsafe syscall with interrupts disabled",
	"Spawn of thread failed",
	"unable to read/write syscall packet",
	"requested thing not found",
	"failed because nothing was requested",
	"not aligned"
};




static inline const char* smk_err( int err )
{
	if ( err < 0 ) err = -err;
	if ( err > SMK_MAXERR ) return "unknown error";
	return smk_errorstrings[ err ];
};



#endif

