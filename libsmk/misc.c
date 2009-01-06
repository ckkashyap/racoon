#include <smk.h>



/**  \defgroup MISC  Miscellaneous Stuff  
 *
 */

/** @{ */



void smk_reboot()
{
	_sysenter( SYS_MISC | SYS_ONE ,0,0,0,0,0 );
}


/** This will abort the running of the current application with
 * the provided reason. Useful for asserts and stuff.
 *
 * \todo Evaluate permanence...
 */
void smk_abort( int rc, const char *reason )
{
	_sysenter( SYS_MISC | SYS_TWO, rc, (int)reason, 0,0,0 );
}



/** This is a semi-permanent request to put the calling
 * thread into a dormant state. While the thread is in a
 * dormant state, it will be removed from the scheduler
 * queue and it won't run. The only way to wake it up
 * would be a state change request by another thread or
 * by sending it an IPC message which it is registered
 * to receive.
 *
 * \return 0
 */
int smk_go_dormant()
{
   return _sysenter( (SYS_THREAD|SYS_FOUR) , 0,0,0,0,0 );
}


/** This request will put the thread into a dormant state
 * for the specified amount of milliseconds. It's not
 * removed from the scheduler queue but put into a 
 * separate queue to allow for fast re-scheduling. Generally,
 * the thread will wake up at the millisecond requested.
 * The scheduler checks on every iteration for dormant
 * threads which need to be woken up.
 *
 * An IPC received by the thread will wake the thread up 
 * prematurely. You can determine if the thread was woken
 * up prematurely because the system call returns the number
 * of milliseconds the thread was dormant.
 *
 * \return The number of milliseconds the thread was dormant.
 * 
 */
int smk_go_dormant_t( int milliseconds )
{
   return _sysenter( (SYS_THREAD|SYS_FOUR) , milliseconds,0,0,0,0 );
}


void smk_exit(int code)
{
	_sysenter( (SYS_PROCESS|SYS_THREE), code, 0, 0,0,0 );
}



/**	Retrieves the LFB information from the kernel if it was
 * started up with GRUB in VBE mode.  If there is no LFB information
 * kept by the kernel, the parameter ptr will be NULL.
 */
void smk_get_lfb( uint32_t** ptr, uint32_t *width, uint32_t *height )
{
	_sysenter( (SYS_MISC|SYS_THREE), 
				(uint32_t)ptr, 
				(uint32_t)width, 
				(uint32_t)height, 0, 0 );
}



/** @} */

