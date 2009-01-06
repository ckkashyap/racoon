#ifndef _KERNEL_RWLOCK_H
#define _KERNEL_RWLOCK_H

#include <smk/atomic.h>

#define		READER			0
#define		WRITER			1

struct rwlock
{
	unsigned int num_readers;
	unsigned int num_writers;
	unsigned int write_request;
	spinlock_t spinlock;
};


struct rwlock *init_rwlock();
int delete_rwlock( struct rwlock * );

int rwlock_get_write_access( struct rwlock * );
int rwlock_get_read_access( struct rwlock * );
int rwlock_release( struct rwlock * );



#endif



