#ifndef _LIBKERNEL_EVENTS_H
#define _LIBKERNEL_EVENTS_H


int smk_set_event_hooks( int handler_tid, 
							int target_pid,
							int target_tid );

#endif

