#ifndef	_LIBKERNEL_MEM_H
#define	_LIBKERNEL_MEM_H


#ifdef __cplusplus
extern "C" {
#endif

#include <smk/inttypes.h>
int smk_mem_alloc(  size_t pages,  void **ptr  );
int smk_mem_free(  void* ptr  );
int smk_mem_size(  void* ptr,  ssize_t *size  );
int smk_mem_location(  void* ptr,  void **location  );
int smk_mem_map(  void* location,  size_t pages,  void **ptr  );


#ifdef __cplusplus
}
#endif


#endif