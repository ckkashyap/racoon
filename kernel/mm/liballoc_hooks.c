#include <smk/atomic.h>
#include <smk/inttypes.h>

#include <mm/mm.h>

#include "liballoc.h"
#include "physmem.h"


static spinlock_t liballoc_spinlock = INIT_SPINLOCK;


int liballoc_lock()
{
	acquire_spinlock( &liballoc_spinlock );
	return 0;
}

int liballoc_unlock()
{
	release_spinlock( &liballoc_spinlock );
	return 0;
}

void* liballoc_alloc(size_t pages)
{
	return memory_alloc( pages );
}

int liballoc_free(void* ptr,size_t pages)
{
	memory_free( pages, ptr );
	return 0;
}



void init_liballoc()
{
	struct mm_subsystem sub;

		sub.malloc = PREFIX(malloc);
		sub.free = PREFIX(free);
		sub.realloc = PREFIX(realloc);

	set_mm_system( sub );
}




