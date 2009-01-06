#include <smk/inttypes.h>

#include <debug/assert.h>
#include "include/interrupts.h"
#include "include/gdt.h"
#include "include/dmesg.h"
#include "include/eflags.h"

#include "mm/physmem.h"
#include "include/cpu.h"

#include "include/apic.h"

int  disable_interrupts()
{
	uint32_t flags = 0;
	asm ( "pushf; pop %%eax; movl %%eax, %0; cli;"
				: "=g" (flags)
				: 
				: "eax"  );
	if ( (flags & EFLAG_INTERRUPT) != 0 ) return 1;
	return 0;
}

void  enable_interrupts()
{
    asm (" sti ");
}



void load_interrupts( int cpu_id )
{
  int i;
  uint32_t ses[2];  

	assert( (cpu_id >= 0) && (cpu_id < MAX_CPU) );

  // Allocate the interrupt table.
	cpu[ cpu_id ].idt = 
		(struct IDT_GATE*)
		memory_alloc( sizeof(struct IDT_GATE) * 256 / 4096 + 1 );

	assert( cpu[ cpu_id ].idt != NULL );
  
  extern void __null_int();

  for (i = 0; i < 256; i++)
    set_interrupt( cpu_id, i, IDT_PRESENT | IDT_32 | IDT_INT, 
			GDT_SYSTEMCODE, (uint32_t)__null_int, IDT_DPL3 );

  extern void __soft_scheduler();
  extern void __apicLVTthermal();
  extern void __apicLVTperfCounter();
  extern void __apicLVTlint0();
  extern void __apicLVTlint1();
  extern void __apicLVTerror();
  extern void __apicLVTspurious();
	
    set_interrupt( cpu_id, APIC_INTERRUPT, IDT_PRESENT | IDT_32 | IDT_INT, 
			GDT_SYSTEMCODE, (uint32_t)__soft_scheduler, IDT_DPL3 );
  
    set_interrupt( cpu_id, APIC_INTERRUPT+1, IDT_PRESENT | IDT_32 | IDT_INT, 
			GDT_SYSTEMCODE, (uint32_t)__apicLVTthermal, IDT_DPL0 );
  
    set_interrupt( cpu_id, APIC_INTERRUPT+2, IDT_PRESENT | IDT_32 | IDT_INT, 
			GDT_SYSTEMCODE, (uint32_t)__apicLVTperfCounter, IDT_DPL0 );
  
    set_interrupt( cpu_id, APIC_INTERRUPT+3, IDT_PRESENT | IDT_32 | IDT_INT, 
			GDT_SYSTEMCODE, (uint32_t)__apicLVTlint0, IDT_DPL0 );
  
    set_interrupt( cpu_id, APIC_INTERRUPT+4, IDT_PRESENT | IDT_32 | IDT_INT, 
			GDT_SYSTEMCODE, (uint32_t)__apicLVTlint1, IDT_DPL0 );
  
    set_interrupt( cpu_id, APIC_INTERRUPT+5, IDT_PRESENT | IDT_32 | IDT_INT, 
			GDT_SYSTEMCODE, (uint32_t)__apicLVTerror, IDT_DPL0 );


	assert( ((APIC_INTERRUPT + 15) & 0xF) == 0xF );

	
	// load the interrupt table
	ses[0] = (256*8) << 16;
	ses[1] = (uint32_t)( cpu[cpu_id].idt );
	asm ("lidt (%0)": :"g" (((char *) ses)+2));

	dmesg( "CPU %i loaded interrupts\n", cpu_id );
}


void set_interrupt( int cpu_id, 
					int num, 
					unsigned int type, 
					unsigned int segment, 
					uint32_t offset, 
					unsigned int level)
{
	assert( (cpu_id >= 0) && (cpu_id < MAX_CPU) );

	cpu[ cpu_id ].idt[num].segment =  segment;
	cpu[ cpu_id ].idt[num].flags = level | type;
	cpu[ cpu_id ].idt[num].offset15_0  = offset & 0xFFFF;
	cpu[ cpu_id ].idt[num].offset16_31 = (offset >> 16) & 0xFFFF;
}


