#ifndef _LIBKERNEL_PULSE_H
#define _LIBKERNEL_PULSE_H

#include <smk.h>

#ifdef __cplusplus
extern "C" {
#endif


int smk_send_pulse( int source_port,
		  int dest_pid, 
		  int dest_port,
		  unsigned int data1,
		  unsigned int data2,
		  unsigned int data3,
		  unsigned int data4,
		  unsigned int data5,
		  unsigned int data6 );

int smk_recv_pulse( int *source_pid, 
		     int *source_port,
		     int *dest_port,
		     unsigned int *data1, 
		     unsigned int *data2, 
		     unsigned int *data3,
		     unsigned int *data4,
		     unsigned int *data5,
		     unsigned int *data6 );

int smk_peek_pulse( int *source_pid,
		     int *source_port,
		     int *dest_port,
		     unsigned int *data1,
		     unsigned int *data2, 
		     unsigned int *data3,
		     unsigned int *data4,
		     unsigned int *data5,
		     unsigned int *data6 );

int smk_waiting_pulses();

int smk_drop_pulse();

#ifdef __cplusplus
}
#endif
	

#endif

