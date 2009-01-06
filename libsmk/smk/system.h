#ifndef	_LIBKERNEL_SYSTEM_H
#define	_LIBKERNEL_SYSTEM_H


#ifdef __cplusplus
extern "C" {
#endif

#include "systemdefs.h"
int smk_system_info(  struct system_info *info  );
int smk_process_info(  int pid,  struct process_info *info  );


#ifdef __cplusplus
}
#endif


#endif