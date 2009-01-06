#ifndef _LIBKERNEL_TIME_H
#define _LIBKERNEL_TIME_H



int 	smk_system_time( unsigned int *seconds, unsigned int *milliseconds);
void 	smk_ndelay( unsigned int milliseconds );
void 	smk_release_timeslice();

#endif

