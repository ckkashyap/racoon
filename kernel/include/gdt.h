#ifndef _KERNEL_GDT_H
#define _KERNEL_GDT_H

#include <smk/inttypes.h>

// DPL level. USER = 3, SYSTEM = 0;
#define G_DPL3        0x60U //1100000 
#define G_DPL1        0x20U //0100000
#define G_DPL2        0x40U //1000000
#define G_DPL0        0x00U //0000000  

// Type:  My normal code = G_USE32 | G_AVAIL
#define G_USE32       0x40U //01000000
#define G_USE16       0x00U //00000000
#define G_GRANULAR    0x80U //10000000
#define G_AVAIL       0x10U //00010000 

// Flags: My normal code = G_CODE |  G_PRESENT | G_READABLE | G_APPLICATION
#define G_SYSTEM       0x00U //00000  TSS. LDT. int. whatever
#define G_APPLICATION  0x10U //10000  application code


// FLAGS -- type of G_SYSTEM segments
#define G_TSS16        0x01U  // 0001 16 bit TSS (Available)
#define G_LDT          0x02U  // 0010 LDT
#define G_TSS16_BUSY   0x03U  // 0011 16 bit TSS (Busy)
#define G_CALL16       0x04U  // 0100 16 bit call gate
#define G_TASK         0x05U  // 0101 Task gate
#define G_INT16        0x06U  // 0110 16 bit interrupt gate
#define G_TRAP16       0x07U  // 0111 16 bit trap gate
#define G_TSS32        0x09U  // 1001 32 bit TSS (available)
#define G_TSS32_BUSY   0x0BU  // 1011 32 bit TSS (busy)
#define G_CALL32       0x0CU  // 1100 32 bit call gate
#define G_INT32        0x0EU  // 1110 32 bit interrupt gate
#define G_TASK_GATE    0x0FU  // 1111 32 bit trap gate

// FLAGS -- type of G_APPLICATION segments
#define G_READABLE     0x02U //00010   (only code)
#define G_CONFORMING   0x04U //00100   (only code)
#define G_CODE         0x08U //01000

#define G_DATA         0x00U //00000
#define G_ACCESSED     0x01U //00001      (both)
#define G_WRITABLE     0x02U //00010   (only data)
#define G_EXPAND_DOWN  0x04U //00100   (only data)
#define G_PRESENT      0x80U //010000000  (both. use for virtual paging & what not)


typedef struct 
{
	unsigned int seg_length0_15	: 16;
	unsigned int base_addr0_15	: 16; 
	unsigned int base_addr16_23	:  8; 
	unsigned int flags			:  8; 
	unsigned int type			:  8;
	unsigned int base_addr24_31	:  8;
} __attribute__ ((packed)) segment_descriptor;



#define GDT_SYSTEMCODE	8
#define GDT_SYSTEMDATA	16
#define GDT_USERCODE	24
#define GDT_USERDATA	32

#define MAX_GDT			256	

#include <smk/inttypes.h>

void 		init_gdt();
uint32_t 	gdt_get_offset( int cpu_id, unsigned int entry );

int 		gdt_new_segment( int cpu_id, 
								uint32_t base, 
							 	uint32_t length, 
								uint8_t flags, 
								uint8_t type, 
							 	uint32_t access);


int 		load_gdt( int cpu_id );

#endif

