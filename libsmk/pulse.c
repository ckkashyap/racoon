#include <smk.h>


/**  \defgroup PULSE  Pulse IPC  
 

<P>
Pulses are the simplest and fastest means of IPC 
available. It allows one process to send 6 values 
(unsigned 32 bit integers) to another process. The 
pulse is asynchronous and non-blocking, meaning 
that if you send it, it's gone and the sending process 
will continue immediately.  Maybe the recipient is going 
to use it, maybe it isn't.

  
 */

/** @{ */

int smk_send_pulse( int source_port,
		  int dest_pid, 
		  int dest_port,
		  unsigned int data1,
		  unsigned int data2,
		  unsigned int data3,
		  unsigned int data4,
		  unsigned int data5,
		  unsigned int data6 )
{
	unsigned int data[6];

	data[0] = data1;
	data[1] = data2;
	data[2] = data3;
	data[3] = data4;
	data[4] = data5;
	data[5] = data6;

	return _sysenter( (SYS_PULSE|SYS_ONE), source_port, dest_pid, dest_port, (unsigned int)&data, 0 );
	
}



/** You need to set *dest_port to -1 before you call this, otherwise,
 * it will check ONLY on port *dest_port for waiting pulses.
 */
int smk_recv_pulse( int *source_pid, 
		     int *source_port,
		     int *dest_port,
		     unsigned int *data1, 
		     unsigned int *data2, 
		     unsigned int *data3,
		     unsigned int *data4,
		     unsigned int *data5,
		     unsigned int *data6 )
{
	unsigned int data[6];
	int ans;

	ans = _sysenter( (SYS_PULSE|SYS_TWO), 
			  (unsigned int)source_pid, 
			  (unsigned int)source_port, 
			  (unsigned int)dest_port,
			  (unsigned int)&data, 0 );

	*data1 = data[0];
	*data2 = data[1];
	*data3 = data[2];
	*data4 = data[3];
	*data5 = data[4];
	*data6 = data[5];

	return ans;
}


/** You need to set *dest_port to -1 before you call this, otherwise,
 * it will check ONLY on port *dest_port for waiting pulses.
 */
int smk_peek_pulse( int *source_pid, 
		     int *source_port,
		     int *dest_port,
		     unsigned int *data1, 
		     unsigned int *data2, 
		     unsigned int *data3,
		     unsigned int *data4,
		     unsigned int *data5,
		     unsigned int *data6 )
{
	unsigned int data[6];
	int ans;

	ans = _sysenter( (SYS_PULSE|SYS_TWO), 
			  (unsigned int)source_pid, 
			  (unsigned int)source_port, 
			  (unsigned int)dest_port,
			  (unsigned int)&data, 0 );

	*data1 = data[0];
	*data2 = data[1];
	*data3 = data[2];
	*data4 = data[3];
	*data5 = data[4];
	*data6 = data[5];

	return ans;
}


int smk_waiting_pulses()
{
	return _sysenter( (SYS_PULSE|SYS_FOUR), 0,0,0,0,0 );
}

int smk_drop_pulse()
{
	return _sysenter( (SYS_PULSE|SYS_FIVE), 0,0,0,0,0 );
}



/** @} */

