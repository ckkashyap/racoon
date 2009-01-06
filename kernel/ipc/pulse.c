#include <processes/threads.h>
#include <scheduler/scheduler.h>
#include "include/smk.h"

#include <ipc/ports.h>
#include <ipc/pulse.h>
#include <ds/aqueue.h>


int send_pulse( int source_pid, 
		     int source_port,
		     struct process *proc, 
		     int dest_port, 
		     unsigned int data1,
		     unsigned int data2,
		     unsigned int data3,
		     unsigned int data4,
		     unsigned int data5,
		     unsigned int data6 )
{
	struct pulse p;
	struct thread *tr;
	

	int tid;

	tid = port2tid( proc, dest_port );		// Find target tid
	if ( tid < 0 ) return -1;

	tr = get_thread( proc, tid );		// Find target port
	if ( tr == NULL ) return -1;

	assert( proc->ipc[dest_port].pulse_q != NULL );	// Allocated

	// Let's get a new pulse to initialize.
	
			p.source_pid   = source_pid;
			p.source_port  = source_port;
			p.dest_port    = dest_port;
			p.data1 = data1;
			p.data2 = data2;
			p.data3 = data3;
			p.data4 = data4;
			p.data5 = data5;
			p.data6 = data6;

		if ( aqueue_add( proc->ipc[dest_port].pulse_q, &p ) != 0 )
			return -1;
	
		
		// WAKE UP THE THREAD IF IT WANTS TO BE WOKEN UP
	   	if ( port_flags( proc, dest_port, IPC_MASKED ) == 0 )
		{
		   if ( tr->state == THREAD_DORMANT )
		   {
		     set_thread_state( tr, THREAD_RUNNING );
		   }
		}

	   
	return 0;
}


int receive_pulse( struct thread *tr,
		     int *source_pid,
		     int *source_port,
		     int *dest_port,
		     unsigned int *data1,
		     unsigned int *data2,
		     unsigned int *data3,
		     unsigned int *data4,
		     unsigned int *data5,
		     unsigned int *data6 )
{
	struct process *proc;
	struct pulse p;
	int port;
	int i;
	

	proc = tr->process;

	// --------- FIRST HONOUR THE LAST PULSE HINT
	
	if ( (*dest_port >= 0) && (*dest_port < MAX_PORTS) )
			port = *dest_port;
		else
			port = tr->last_port_pulse;

	tr->last_port_pulse = -1;

	if ( (port >= 0) && (port < MAX_PORTS) )
	{
				
        if ( port_flags( proc, port, IPC_MASKED ) != 0 ) return -1;

		assert( proc->ipc[port].pulse_q != NULL );	// Allocated

		if ( aqueue_count( proc->ipc[port].pulse_q ) == 0 ) 
			return -1;

			i = aqueue_next( proc->ipc[port].pulse_q, &p );
			assert( i != 0 );

			*source_pid  = p.source_pid;
			*source_port = p.source_port;
			*dest_port   = p.dest_port;
		
			*data1 = p.data1;
			*data2 = p.data2;
			*data3 = p.data3;
			*data4 = p.data4;
			*data5 = p.data5;
			*data6 = p.data6;

		return 0;			  
	  }
	

	// --------------------------- NOW DO THE REST OF THEM	
	
	for ( i = 0; i < MAX_PORTS; i++ )
	{
		if ( tr->ports[i] == -1 ) break;

		// -------------------------------------------
		port = tr->ports[i];
		if ( port_flags( proc, port, IPC_MASKED ) != 0 ) continue;

		assert( proc->ipc[port].pulse_q != NULL );	// Allocated

		if ( aqueue_count( proc->ipc[port].pulse_q ) == 0 ) 
			return -1;

			i = aqueue_next( proc->ipc[port].pulse_q, &p );
			assert( i != 0 );

			*source_pid  = p.source_pid;
			*source_port = p.source_port;
			*dest_port   = p.dest_port;
		
			*data1 = p.data1;
			*data2 = p.data2;
			*data3 = p.data3;
			*data4 = p.data4;
			*data5 = p.data5;
			*data6 = p.data6;

		return 0;
	}

	return -1;
}



int preview_pulse( struct thread *tr,
		     int *source_pid, 
		     int *source_port,
		     int *dest_port,
		     unsigned int *data1,
		     unsigned int *data2,
		     unsigned int *data3,
		     unsigned int *data4,
		     unsigned int *data5,
		     unsigned int *data6 )
{
	struct process *proc;
	struct pulse p;
	int port;
	int top, bottom;
	int i;


	proc = tr->process;

	if ( (*dest_port >= 0) && (*dest_port < MAX_PORTS) )
	{
		bottom = -1;
		for ( i = 0; i < MAX_PORTS; i++ )
		{
			if ( tr->ports[i] == -1 ) return -1;	 // Ran out of ports

			if ( tr->ports[i] == *dest_port )
			{
				bottom = i;
				top = i + 1;
				break;
			}
		}
		if ( bottom == -1 ) return -1;
	}
	else
	{
		bottom = 0;
		top = MAX_PORTS;
	}

	tr->last_port_pulse = -1;
	
	for ( i = bottom; i < top; i++ )
	{
	    if ( tr->ports[i] == -1 ) break;

	    port = tr->ports[i];
		if ( port_flags( proc, port, IPC_MASKED ) != 0 ) continue;

		assert( proc->ipc[port].pulse_q != NULL );	// Allocated

			if ( aqueue_count( proc->ipc[port].pulse_q ) == 0 )
				return -1;

			i = aqueue_next( proc->ipc[port].pulse_q, &p );
			assert( i == 0 );

			*source_pid  = p.source_pid;
			*source_port = p.source_port;
			*dest_port   = p.dest_port;
		
			*data1 = p.data1;
			*data2 = p.data2;
			*data3 = p.data3;
			*data4 = p.data4;
			*data5 = p.data5;
			*data6 = p.data6;

		tr->last_port_pulse = port;	// hint for drop/receive
		return 0;
	}

	return -1;
}



int pulses_waiting( struct thread *tr )
{
	struct process *proc;
	int count;
	int i;

	count = 0;

	proc = tr->process;
	
	for ( i = 0; i <  MAX_PORTS; i++ )
	{
	  if ( tr->ports[i] == -1 ) break;
	  if ( port_flags( proc, tr->ports[i], IPC_MASKED ) != 0 ) continue;

	  assert( proc->ipc[ tr->ports[i] ].pulse_q != NULL );	// Allocated
	  
	  count += aqueue_count( proc->ipc[ tr->ports[i] ].pulse_q );
    }

	return count;
}


int drop_pulse( struct thread *tr )
{
	struct process *proc;
	struct pulse p;
	int port;
	int i;

	proc = tr->process;
	

	// --------- FIRST HONOUR THE LAST PULSE HINT
	
	port = tr->last_port_pulse;
	tr->last_port_pulse = -1;

	if ( (port >= 0) && (port < MAX_PORTS) )
	  {
	      if ( port_flags( proc, port, IPC_MASKED ) != 0 ) return -1;

	  		assert( proc->ipc[port].pulse_q != NULL );	// Allocated

			aqueue_next( proc->ipc[port].pulse_q, &p );
			
		return 0;			  
	}
	

	// --- NOW DO THE REST OF THEM IF NOTHING IS MARKED
	
	for ( i = 0; i < MAX_PORTS; i++ )
	{
		if ( tr->ports[i] == -1 ) break;

		// -------------------------------------------
		
		port = tr->ports[i];


		aqueue_next( proc->ipc[port].pulse_q, &p );
		  
		return 0;
	}


	return -1;
}


