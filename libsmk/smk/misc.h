#ifndef _LIBKERNEL_MISC_H
#define _LIBKERNEL_MISC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <smk.h>


		
	void smk_reboot();
	void smk_exit(int code);
	void smk_abort( int rc, const char *reason );
  
	int smk_go_dormant();
	int smk_go_dormant_t( int milliseconds );

	void smk_get_lfb( uint32_t** ptr, uint32_t *width, uint32_t *height );


#ifdef __cplusplus
}       
#endif
#endif

