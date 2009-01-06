#include <smk/inttypes.h>
#include <smk/strings.h>
#include "include/dmesg.h"

#include "include/interrupts.h"
#include "mm/multiboot.h"
#include "include/misc.h"
#include "include/gdt.h"
#include "mm/physmem.h"
#include "include/paging.h"

#include "include/syscalls.h"

#include "include/modules.h"

#include "include/cpu.h"
#include "include/time.h"
#include "include/exceptions.h"

#include "include/irq.h"
#include "include/ioapic.h"

#include "include/apic.h"


#include "include/smp.h"

#include <processes/process.h>

#include "include/smk.h"

#include "include/shmem.h"

#include <mm/liballoc_hooks.h>
#include <mm/bootstrap.h>

#include <mm/vmem.h>


void init( unsigned int magic, multiboot_info_t *mb_info )
{
	kzero_bss();

	init_gdt();

	if ( magic != MULTIBOOT_BOOTLOADER_MAGIC )      
	{
		while (1==1) ((char*)0xB8000)[0]++; 
	}
	
	dmesg("%!spoon microkernel %s built at %s, %s\n", KERNEL_VERSION, __DATE__,  __TIME__ );

	bootstrap_memory( mb_info );

	init_liballoc();

	init_paging();

	init_ioapic();

	init_irqs();

	init_time();

	init_cpus();

	shmem_init();

	init_processes();

	init_smk();

	init_modules( mb_info );

	start_ap_processors();

	cpu_init( 0 );	// Take a short-cut for the BSP processor.
}









