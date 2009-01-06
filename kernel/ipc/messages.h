#ifndef _KERNEL_MESSAGES_H
#define _KERNEL_MESSAGES_H

#include <processes/process.h>


#define	MESSAGE_OBJECTS		256


struct message
{
  int source_pid;
  int source_port;
  int dest_port;
  int bytes;
  void *data;
} __attribute__ ((packed));




int send_message( int source_pid,
		  int source_port,
		  struct process *proc,
		  int dest_port,
		  int bytes,
		  void *location );

int recv_message( struct thread* tr,
		  int *source_pid,
		  int *source_port,
		  int *dest_port,
		  int bytes,
		  void *location );

int peek_message( struct thread* tr,
		  int *source_pid,
		  int *source_port,
		  int *dest_port,
		  int *bytes );

int messages_waiting( struct thread *tr );

int drop_message( struct thread* tr );


#endif

