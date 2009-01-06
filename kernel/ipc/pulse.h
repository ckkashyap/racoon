#ifndef _KERNEL_PULSE_H
#define _KERNEL_PULSE_H

#include <smk/inttypes.h>
#include <processes/threads.h>



#define	PULSE_OBJECTS		1024


struct pulse
{
	int source_pid;
	int source_port;
	int dest_port;
	unsigned int data1;
	unsigned int data2;
	unsigned int data3;
	unsigned int data4;
	unsigned int data5;
	unsigned int data6;
};



int send_pulse( int source_pid, 
			     int source_port,
			     struct process *proc, 
			     int dest_port, 
			     unsigned int data1,
			     unsigned int data2,
			     unsigned int data3,
			     unsigned int data4,
			     unsigned int data5,
			     unsigned int data6 );

int receive_pulse( struct thread *tr,
		     int *source_pid,
		     int *source_port,
		     int *dest_port,
		     unsigned int *data1,
		     unsigned int *data2,
		     unsigned int *data3,
		     unsigned int *data4,
		     unsigned int *data5,
		     unsigned int *data6 );

int preview_pulse( struct thread *tr,
		     int *source_pid,
		     int *source_port,
		     int *dest_port,
		     unsigned int *data1,
		     unsigned int *data2,
		     unsigned int *data3,
		     unsigned int *data4,
		     unsigned int *data5,
		     unsigned int *data6 );


int pulses_waiting( struct thread *tr );
int drop_pulse( struct thread *tr );

#endif


