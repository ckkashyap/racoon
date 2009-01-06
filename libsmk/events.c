#include <smk.h>


/**  \defgroup EVENTS  Thread Control  
 
<P>
Events are the way of the kernel informing userland applications of
certain things which happen in the system. I'm still formulating it.
<p>
 
*/

/** @{ */


int smk_set_event_hooks( int handler_tid, 
							int target_pid,
							int target_tid )
{
  return _sysenter( (SYS_EVENTS|SYS_ONE), 
	      			   (uint32_t)handler_tid, 
						 (uint32_t)target_pid, 
						 (uint32_t)target_tid,
						 0,
						 0);
 
}


/** @} */

