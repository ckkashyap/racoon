#include <mm/mm.h>



static struct mm_subsystem subsystem;


void set_mm_system( struct mm_subsystem sub )
{
	subsystem = sub;
}



void* kmalloc(size_t s)
{
	return subsystem.malloc(s);
}

void kfree(void* ptr)
{
	return subsystem.free(ptr);
}

void* krealloc(void* ptr,size_t s)
{
	return subsystem.realloc(ptr,s);
}


