#include <smk.h>




/**  \defgroup PROCESS  Process Control  
 
In the context of the spoon microkernel, a process is an abstract
concept to describe a collection of threads which share the same
memory space and process id. (and other stuff, like security permissions, etc)

<P>
In truth, that's about it.

<p>
When a new process is created, a memory space is created and a main thread
is spawned which is given the thread id of 0.  This main thread
may then spawn more threads or function purely on it's own. It's 
the program's choice.

  
 */

/** @{ */

static int _libsmk_cached_pid = -1;


int smk_find_process_id( const char *name )
{
	return _sysenter( (SYS_SYSTEM|SYS_ONE), 0, (unsigned int)name,0,0,0 );
}


int smk_find_process_name( int pid, const char *name )
{
	return _sysenter( (SYS_SYSTEM|SYS_TWO), pid, (unsigned int)name, 0,0,0 );
}

int smk_process_exists( int pid )
{
	return _sysenter( (SYS_SYSTEM|SYS_THREE), pid, 0,0,0,0 );
}



int smk_getpid()
{
	int a;
	int b;

	if ( _libsmk_cached_pid > 0 ) return  _libsmk_cached_pid;
	_sysenter( (SYS_PROCESS|SYS_ONE),(unsigned int)&a,(unsigned int)&b,0,0,0 );
	_libsmk_cached_pid = a;
    return a;
}

int smk_start_process( int pid )
{
	return _sysenter( (SYS_PROCESS|SYS_TWO),pid,0,0,0,0 );
}

/** @} */

