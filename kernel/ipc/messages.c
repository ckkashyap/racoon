#include <smk/strings.h>
#include <mm/mm.h>

#include "include/smk.h"
#include "include/misc.h"
#include <processes/process.h>
#include <processes/threads.h>
#include <ipc/messages.h>
#include <ipc/ports.h>
#include <ipc/ipc.h>


#include <ds/aqueue.h>

// Currently, the whole kernel needs to be locked
// for these functions to be safe...


int send_message( int source_pid,
				  int source_port,
				  struct process* proc,
				  int dest_port,
				  int bytes,
				  void *location )
{
	struct message msg;
	struct thread *tr;
	int dest_tid;
	void *data;


	dest_tid = port2tid( proc, dest_port );
	if ( dest_tid < 0 ) return -1;

	tr = get_thread( proc, dest_tid );
	if ( tr == NULL ) return -1;

	data = kmalloc( bytes );
	if ( data == NULL ) return -1;
	

		msg.source_pid   	= source_pid;
		msg.source_port  	= source_port;
		msg.dest_port    	= dest_port;
		msg.bytes 			= bytes;
		msg.data  			= data;

		kmemcpy( msg.data, location, bytes );

	if ( aqueue_add( proc->ipc[dest_port].message_q, &msg ) != 0 )
	{
		kfree( data );
		return -1;
	}


	// WAKE UP THE THREAD, maybe

   	if ( port_flags( proc, dest_port, IPC_MASKED ) == 0 )
	{
	   if ( tr->state == THREAD_DORMANT )
	   {
		set_thread_state( tr, THREAD_RUNNING );
	   }

	}

	return 0;
}


int recv_message( struct thread* tr,
		  int *source_pid,
		  int *source_port,
		  int *dest_port,
		  int bytes,
		  void *location )
{
	struct message tmp;
	int size;
	int i;
	int port;
	struct process *proc = tr->process;


	// --------- First obey the hint supplied in thread structure
	

	if ( (*dest_port >= 0) && (*dest_port < MAX_PORTS) )
			port = *dest_port;
		else
			port = tr->last_port_message;

	tr->last_port_message = -1;

	
	if ( (port >= 0) && (port < MAX_PORTS) )
	{

		if ( port_flags( proc, port, IPC_MASKED ) != 0 ) return -1;
		assert( proc->ipc[port].message_q != NULL );
		 
		if ( aqueue_count( proc->ipc[port].message_q ) == 0 ) 
			return -1;

		i = aqueue_next( proc->ipc[port].message_q, &tmp );
		assert( i == 0 );
	   
		size = tmp.bytes;
		if ( bytes < size ) size = bytes;
		
		*source_pid  = tmp.source_pid;
		*source_port = tmp.source_port;
		*dest_port   = tmp.dest_port;
		
		kmemcpy( location, tmp.data, size );
		kfree( tmp.data );

		return size;
   }


	// ------ NOW SEARCH EVERYTHING -------------------------
	
	for ( i = 0; i < MAX_PORTS; i++ )
	{
		if ( tr->ports[i] == -1 ) break;

		port = tr->ports[i];

		if ( port_flags( proc, port, IPC_MASKED ) != 0 ) continue;
		assert( proc->ipc[port].message_q != NULL );

		// ----- message to return

		if ( aqueue_count( proc->ipc[port].message_q ) == 0 ) 
			continue;

		i = aqueue_next( proc->ipc[port].message_q, &tmp );
		assert( i == 0 );
	
			
			size = tmp.bytes;
			if ( bytes < size ) size = bytes;
			
			*source_pid  = tmp.source_pid;
			*source_port = tmp.source_port;
			*dest_port   = tmp.dest_port;
			
			kmemcpy( location, tmp.data, size );
			kfree( tmp.data );

		return size;
	}
		
	return -1;
}


#warning This drops messages.. it's just receive.
int peek_message( struct thread* tr,
		  int *source_pid,
		  int *source_port,
		  int *dest_port,
		  int *bytes )
{

	struct message tmp;
	int i;
	int port;
	int top, bottom;
	struct process *proc = tr->process;


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
		top = MAX_PORTS;
		bottom = 0;
	}



	tr->last_port_message = -1;

	for ( i = bottom; i < top; i++ )
	{
		if ( tr->ports[i] == -1 ) break;
		port = tr->ports[i];

		if ( port_flags( proc, port, IPC_MASKED ) != 0 ) continue;
		assert( proc->ipc[port].message_q != NULL );


		// ----- message to return


		if ( aqueue_count( proc->ipc[port].message_q ) == 0 ) 
			continue;

		i = aqueue_next( proc->ipc[port].message_q, &tmp );
		assert( i == 0 );
	

			tr->last_port_message = port;
		   
			
			*source_pid  = tmp.source_pid;
			*source_port = tmp.source_port;
			*dest_port   = tmp.dest_port;
			*bytes	     = tmp.bytes;

		return 0;
	}
		
	return -1;
}



int messages_waiting( struct thread *tr )
{
	int count;
	int port;
	int i;
	struct process *proc = tr->process;

	count = 0;
	
	for ( i = 0; i < MAX_PORTS; i++ )
	{
		if ( tr->ports[i] == -1 ) break;

		port = tr->ports[i];
		if ( port_flags( proc, port, IPC_MASKED ) != 0 ) continue;

		assert( proc->ipc[port].message_q != NULL );

		count += aqueue_count( proc->ipc[port].message_q );
	}
		
	return count;
}




int drop_message( struct thread *tr )
{
	struct message tmp;
	int port;
	int i;
	struct process *proc = tr->process;


	// ----- First honour the suggested port in the thread structure

	port = tr->last_port_message;
	tr->last_port_message = -1;

	if ( (port >= 0) && (port < MAX_PORTS) )
	{
	  
		if ( port_flags( proc, port, IPC_MASKED ) != 0 ) return -1;
		
		assert( proc->ipc[port].message_q != NULL );
		// ----- message to return

		if ( aqueue_count( proc->ipc[port].message_q ) == 0 ) return -1;
		
			i = aqueue_next( proc->ipc[port].message_q, &tmp );
			assert( i == 0 );
			kfree( tmp.data );

		return 0;
    }


	// ------------ NOW SCAN THROUGH THEM ALL -------------------
	
	for ( i = 0; i < MAX_PORTS; i++ )
	{
		if ( tr->ports[i] == -1 ) break;

		port = tr->ports[i];

		if ( port_flags( proc, port, IPC_MASKED ) != 0 ) continue;

		assert( proc->ipc[port].message_q != NULL );

		// ----- message to return

		if ( aqueue_count( proc->ipc[port].message_q ) == 0 ) return -1;
		
			i = aqueue_next( proc->ipc[port].message_q, &tmp );
			assert( i == 0 );
			kfree( tmp.data );

		return 0;
	}
		
	return -1;
}


