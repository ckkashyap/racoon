#include <smk.h>



/**  \defgroup MESSAGES  Message IPC  



<P>
Messages are a way of sending an arbitrary piece of 
data between two processes or threads in a system. 
If you just want to send a block of information, then 
this IPC method would be it.

 
 */

/** @{ */

int smk_send_message( int source_port, 
					  int dest_pid,
					  int dest_port,
					  int bytes, void *data )
{
 return _sysenter( SYS_MESSAGE | SYS_ONE,
  		    source_port,
		    dest_pid,
		    dest_port,
		    bytes, 
		    (unsigned int)data    );
}



/** You need to set *dest_port to -1 before you call this, otherwise,
 * it will check ONLY on port *dest_port for waiting messages.
 */
int smk_recv_message( int *source_pid,
			          int *source_port,
					  int *dest_port,
					  int bytes, 
					  void *data )
{
  return _sysenter( SYS_MESSAGE | SYS_TWO,
  		    (unsigned int)source_pid, 
		    (unsigned int)source_port,
		    (unsigned int)dest_port,
		    bytes, 
		    (unsigned int)data );
}


/** You need to set *dest_port to -1 before you call this, otherwise,
 * it will check ONLY on port *dest_port for waiting messages.
 *
 *  \return 0 if there is a message waiting
 */
int smk_peek_message( int *source_pid,
			          int *source_port,
					  int *dest_port,
					  int *bytes )
{

  return _sysenter( SYS_MESSAGE | SYS_THREE,
  		    (unsigned int)source_pid, 
		    (unsigned int)source_port,
		    (unsigned int)dest_port,
		    (unsigned int)bytes, 
		    0 );
}




int smk_drop_message()
{
  return _sysenter( SYS_MESSAGE | SYS_FOUR,
  		    0, 
		    0, 
		    0,
		    0, 
		    0 );
}


/** @} */

