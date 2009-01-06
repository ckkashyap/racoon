#ifndef _LIBKERNEL_PROCESS_H
#define _LIBKERNEL_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

int smk_find_process_id( const char *name );
int smk_find_process_name( int pid, const char *name );

int smk_process_exists( int pid );

int smk_getpid();
int smk_start_process( int pid );


#ifdef __cplusplus
}
#endif



#endif

