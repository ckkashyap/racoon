#include <smk.h>


static volatile unsigned int innerLock = 0;


int liballoc_lock()
{
	acquire_spinlock( &innerLock );
	return 0;
}

int liballoc_unlock()
{
	release_spinlock( &innerLock );
	return 0;
}

void* liballoc_alloc(int pages)
{
	return smk_mem_alloc( pages );
}

int liballoc_free(void* ptr,int pages)
{
	smk_mem_free( ptr );
	return 0;
}


