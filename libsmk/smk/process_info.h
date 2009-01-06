#ifndef _LIBKERNEL_PROCESS_INFO_H
#define _LIBKERNEL_PROCESS_INFO_H


#define PROCESS_INIT			1
#define PROCESS_RUNNING			2
#define PROCESS_STOPPED			3
#define PROCESS_FINISHING		4
#define PROCESS_FINISHED		5
#define PROCESS_CRASHING		6
#define PROCESS_CRASHED			7


#ifdef __cplusplus
extern "C" {
#endif

const char*	smk_process_state_str( int state );


#ifdef __cplusplus
}
#endif

#endif

