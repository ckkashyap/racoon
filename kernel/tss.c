#include <smk/inttypes.h>
#include <smk/strings.h>
#include <debug/assert.h>
#include "include/tss.h"
#include "include/eflags.h"
#include "include/paging.h"
#include "include/gdt.h"
#include "include/misc.h"

#include "mm/physmem.h"


extern void __tasking();



/** This function is primarily used to allocate a TSS for each
 * new CPU that is found.
 */
struct TSS* new_cpu_tss( int stack_pages )
{
	uint32_t stack = 0;
	int size;
	if ( stack_pages != 0 ) 
	{
		stack = (uint32_t)memory_alloc( stack_pages );
		stack = stack + stack_pages * 4096;
	}
	
	assert( (sizeof(struct TSS) % PAGE_SIZE) != 0 );
	size = sizeof( struct TSS ) / PAGE_SIZE + 1;

	struct TSS *tss = (struct TSS*)memory_alloc( size );
	if ( tss == NULL ) return NULL;
		
	kmemset( tss, 0, sizeof( struct TSS ) );

			tss->esp0		 = stack;
			tss->ss0		 = GDT_SYSTEMDATA;
			tss->ss1		 = GDT_SYSTEMDATA | 1;
			tss->ss2		 = GDT_SYSTEMDATA | 2;
			tss->cr3		 = (uint32_t)&page_directory;
			tss->eip		 = (uint32_t)__tasking;
			tss->eflags		 = EFLAG_BASE;
			tss->esp		 = stack;
			tss->es			 = GDT_SYSTEMDATA;
			tss->cs			 = GDT_SYSTEMCODE;
			tss->ss			 = GDT_SYSTEMDATA;
			tss->ds			 = GDT_SYSTEMDATA;
			tss->fs			 = GDT_SYSTEMDATA;
			tss->gs			 = GDT_SYSTEMDATA;

	tss->iomapbase =  (uint16_t)(((uint32_t)&(tss->iobitmap)) - (uint32_t)tss);

	return tss;
}

