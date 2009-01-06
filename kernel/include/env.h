#ifndef _KERNEL_ENV_H
#define _KERNEL_ENV_H

#include <smk/limits.h>
#include <smk/inttypes.h>

/// Declaration
struct process;



int	set_environment( struct process *proc, const char name[SMK_ENVNAME_LENGTH], const void *data, size_t size ); 
int get_environment( struct process *proc, const char name[SMK_ENVNAME_LENGTH], void *data, size_t *size ); 
int get_environment_size( struct process* proc, const char name[SMK_ENVNAME_LENGTH], size_t *size );
int get_environment_information( struct process* proc, int id, char name[SMK_ENVNAME_LENGTH], size_t *size );
int remove_environment( struct process *proc, const char name[SMK_ENVNAME_LENGTH] );


int clone_environment( struct process *dest, struct process *src );
int release_environment( struct process *proc );


#endif

