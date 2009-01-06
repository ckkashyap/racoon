#ifndef	_LIBKERNEL_ENV_H
#define	_LIBKERNEL_ENV_H


#ifdef __cplusplus
extern "C" {
#endif

#include <smk/inttypes.h>
int smk_setenv(  const char name[SMK_ENVNAME_LENGTH],  void *data,  size_t size  );
ssize_t smk_getenv(  const char name[SMK_ENVNAME_LENGTH],  void *data,  size_t size  );
ssize_t smk_getenv_size(  const char name[SMK_ENVNAME_LENGTH]  );
int smk_getenv_info(  int i,  const char name[SMK_ENVNAME_LENGTH],  size_t *size  );
int smk_clearenv(  const char name[SMK_ENVNAME_LENGTH]  );


#ifdef __cplusplus
}
#endif


#endif