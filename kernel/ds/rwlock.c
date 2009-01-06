#include <smk/atomic.h>
#include <mm/mm.h>

#include <ds/rwlock.h>


/* A reader-writer locking implementation.  */


struct rwlock *init_rwlock()
{
	struct rwlock *rw = (struct rwlock*)kmalloc(sizeof(struct rwlock));

		rw->num_readers   = 0;
		rw->num_writers   = 0;
		rw->write_request = 0;
		rw->spinlock = INIT_SPINLOCK;

	return rw;
}


int delete_rwlock( struct rwlock *rw )
{
	kfree( rw );
	return 0;
}

int rwlock_get_write_access( struct rwlock *rw )
{
	while ( 1==1 )
	{
		acquire_spinlock( &(rw->spinlock) );

			if ( (rw->num_readers == 0) && (rw->num_writers == 0) )
			{
				atomic_inc( &(rw->num_writers) );
				release_spinlock( &(rw->spinlock) );
				break;
			}
			else rw->write_request = 1;
	
		release_spinlock( &(rw->spinlock) );
	}

	return 0;
}


int rwlock_get_read_access( struct rwlock *rw )
{
	
	while ( 1==1 )
	{
		acquire_spinlock( &(rw->spinlock) );

			if (  (rw->num_writers == 0) && (rw->write_request == 0) )
			{
				rw->num_readers += 1;
				release_spinlock( &(rw->spinlock) );
				break;
			}
	
		release_spinlock( &(rw->spinlock) );
	}
	
	return 0;

}

int rwlock_release( struct rwlock *rw )
{
	if ( rw->num_writers != 0 )
	{
		rw->write_request = 0;
		rw->num_writers   = 0;
	}
	else
	{
		atomic_dec( &(rw->num_readers) );
	}

	return 0;
}






