#include <smk/inttypes.h>
#include <smk/strings.h>

#include "include/apic.h"
#include "include/misc.h"
#include "include/dmesg.h"


#define BOOT_ADDRESS		0x7c000
	
void start_ap_processors()
{
	uint64_t IPI;
	/** Load the __ap_boot over the old boot sector. */

	extern void __ap_boot();
	kmemcpy( (char*)BOOT_ADDRESS, __ap_boot, 4096 );
	
	dmesg("Waking AP processors\n");
	
	
	/** Initialize the other APs to start at __ap_boot (see switch.S) */

	IPI = (0xFFull << 56 ) | (3<<18) | (1<<14) | (6<<8) | (0x7c);

	APIC_SET( APIC_highICR, (uint32_t)(IPI>>32) ); 
	APIC_SET( APIC_lowICR,  (uint32_t)(IPI & 0xFFFFFFFF) ); 
}



