#include <mm/mm.h>
#include <processes/threads.h>
#include <processes/process.h>
#include "include/smk.h"
#include <ipc/ipc.h>
#include <ipc/ports.h>
#include <ipc/pulse.h>
#include <ipc/messages.h>



// For use in the bulk set/get syscalls
struct port_pair
{
	int port;
	unsigned int flags;
} __attribute__ ((packed));

// ----------------------------------------------------

int create_port( struct thread *tr, int port, int tid )
{
  int i;
  struct process *proc = tr->process;
  struct thread *target;
  
  if ( (port < 0) || (port>=MAX_PORTS) ) return -1;

  if ( proc->ipc[port].tid != -1 ) return -1;


  if ( tr->tid == tid ) target = tr;
	  	   else target = get_thread( proc, tid );


  if ( target == NULL ) return -1;
  

  // Queue Initialization

  proc->ipc[port].pulse_q = aqueue_create( sizeof(struct pulse), PULSE_OBJECTS );
  if ( proc->ipc[port].pulse_q == NULL ) return -1;

  proc->ipc[port].message_q = aqueue_create( sizeof(struct message), MESSAGE_OBJECTS );
  if ( proc->ipc[port].message_q == NULL ) 
  {
  		aqueue_destroy( proc->ipc[port].pulse_q );
		proc->ipc[port].pulse_q = NULL;
		return -1;
  }

  // --------------

  proc->ipc[port].tid 			= tid;
  proc->ipc[port].flags 		= 0;
  
  for ( i = 0; i < MAX_PORTS; i++ )
	if ( target->ports[i] == -1 ) 
	{
		target->ports[i] = port;
		break;
	}

  
  return 0;
}

int port2tid( struct process *proc, int port )
{
  if ( (port < 0) || ( port >= MAX_PORTS ) ) return -1;
  return proc->ipc[port].tid;
}

int tid2port( struct process *proc, int tid )
{
  int i;
   
   for ( i = 0; i < MAX_PORTS; i++ )
     if ( proc->ipc[i].tid == tid ) 
     {
       return i;
     }

   return -1;
}


int port_flags( struct process *proc, int port, unsigned int flags )
{
  if ( (port < 0) || (port >= MAX_PORTS) ) return -1;
 
  if ( proc->ipc[port].tid == -1 ) return -1;

  return ( proc->ipc[port].flags & flags );
}


int get_port_flags( struct thread *tr, int port )
{
  struct process *proc;
  proc = tr->process;

  if ( (port < 0) || (port >= MAX_PORTS) ) return -1;
  if ( proc->ipc[port].tid == -1 ) return -1;
  
  return ( proc->ipc[port].flags );
}

int set_port_flags( struct thread *tr, int port, unsigned int flags )
{
  struct process *proc;
  int i;
  int p;
  proc = tr->process;
  
  if ( (port < -1) || (port >= MAX_PORTS) ) return -1;
  
  tr->last_port_message = -1;
  tr->last_port_pulse = -1;
  
  if ( port >= 0 )
  {
  	if ( proc->ipc[port].tid == -1 ) return -1;
 	proc->ipc[port].flags = flags;
	return 0;
  }

  // -1 is all ports per thread.
  for ( i = 0; i < MAX_PORTS; i++ )
  {
    if ( tr->ports[i] == -1 ) break;
    p = tr->ports[i];
    proc->ipc[ p ].flags = flags;
  }
  
  
  return 0;
}



int release_port( struct thread *tr, int port )
{
  int i;
  int found = 0;
  struct process *proc = tr->process;
  struct thread *target = NULL;
  
   
  if ( port < 0 ) return -1;
  if ( port >= MAX_PORTS ) return -1;
  if ( proc->ipc[port].tid == -1 ) return -1;

  if ( tr->tid == proc->ipc[port].tid ) target = tr;
	  else target = get_thread( proc, proc->ipc[port].tid );


	if ( target != NULL )
	{
		// Drop whatever was there.
			
		do { target->last_port_message = port; } 
		while ( drop_pulse( target ) >= 0 );
		
		do { target->last_port_message = port; }
		while ( drop_message( target ) >= 0 ) ;

	}


	aqueue_destroy( proc->ipc[port].pulse_q );	// Clean the pulse queue
	aqueue_destroy( proc->ipc[port].message_q );	// Clean the pulse queue
		
    proc->ipc[port].tid 		 = -1;
    proc->ipc[port].flags 		 = 0;
    proc->ipc[port].pulse_q		 = NULL;
    proc->ipc[port].message_q	 = NULL;


	if ( target != NULL )
		for ( i = 0; i < MAX_PORTS; i++ )
		{
			if ( target->ports[i] == -1 ) break;
			if ( target->ports[i] == port )
			{
				target->ports[i] = -1;
				found = 1;
			}
				  
			if (  ( (found == 1) && (i == (MAX_PORTS-1) ) ) )
			     target->ports[i] = -1;
			else 
				target->ports[i] = target->ports[i + found];
		}


  return 0;
}


int bulk_get_ports( struct thread *tr, void *data )
{
	int i = 0; 
	struct port_pair *pairs = (struct port_pair*)data;

	for ( i = 0; i < MAX_PORTS; i++ )
	{
		pairs[i].port = tr->ports[i];
		if ( tr->ports[i] == -1 ) break;	
		pairs[i].flags = get_port_flags( tr, tr->ports[i] );
	}

	return 0;
}

int bulk_set_ports( struct thread *tr, void *data )
{
	int i = 0; 
	struct port_pair *pairs = (struct port_pair*)data;

	for ( i = 0; i < MAX_PORTS; i++ )
	{
		if ( pairs[i].port == -1 ) break;	
		set_port_flags( tr, pairs[i].port, pairs[i].flags );
	}

	return 0;
}


int release_thread_ports( struct thread *tr )
{
  int count = 0;

  while ( tr->ports[0] != -1 )
  {
     release_port( tr, tr->ports[0] );
     count++;
  }
  
  return count;
}

