#ifndef _KERNEL_GC_H
#define _KERNEL_GC_H


void gc();


int  	gc_queue( int pid, int tid );
int 	gc_has_work( int cpu_id );
		


void load_gc( int cpu_id );



#endif

