#include <smk/inttypes.h>
#include "include/dmesg.h"
#include "include/io.h"


int usingIOAPIC = 0;

#define		IOAPIC_BASE		0xFEC00000
#define		IOAPIC_WINDOW	0xFEC00010

#define		REG_ID				0x00
#define 	REG_VERSION			0x01
#define		REG_ARBITRATION		0x02

#define		REG_INT0			0x10
#define		REG_INT1			0x12
#define		REG_INT2			0x14
#define		REG_INT3			0x16
#define		REG_INT4			0x18
#define		REG_INT5			0x1A
#define		REG_INT6			0x1C
#define		REG_INT7			0x1E
#define		REG_INT8			0x20
#define		REG_INT9			0x22
#define		REG_INT10			0x24
#define		REG_INT11			0x26
#define		REG_INT12			0x28
#define		REG_INT13			0x2A
#define		REG_INT14			0x2C
#define		REG_INT15			0x2E
#define		REG_INT16			0x30
#define		REG_INT17			0x32
#define		REG_INT18			0x34
#define		REG_INT19			0x36
#define		REG_INT20			0x38
#define		REG_INT21			0x3A
#define		REG_INT22			0x3C
#define		REG_INT23			0x3E

static void write_ioapic( uint32_t reg, uint32_t value )
{
	*((uint32_t*)IOAPIC_BASE) = reg;
	*((uint32_t*)IOAPIC_WINDOW) = value;
}

static uint32_t read_ioapic( uint32_t reg )
{
	*((uint32_t*)IOAPIC_BASE) = reg;
	return *((uint32_t*)IOAPIC_WINDOW);
}


static void disable_legacy_pic()
{
	outb_p( 0x21, 0xFF );
	outb_p( 0xA1, 0xFF );
}


static void enable_ioapic()
{
	outb_p( 0x22, 0x70 );
	outb_p( 0x23, 0x01 );
}

void mask_ioapic( int num )
{
	uint32_t type = (1<<16);

	if ( num > 15 )
		type = type | (1<<15) | (1<<13);

	write_ioapic( num*2 + REG_INT0, type | (0x20 + num) );
	write_ioapic( num*2 + REG_INT0+1, 0 );
}

void unmask_ioapic( int num )
{
	uint32_t type = 0;

	if ( num > 15 )
		type = (1<<15) | (1<<13);

	write_ioapic( num*2 + REG_INT0, type | (0x20 + num) );
	write_ioapic( num*2 + REG_INT0+1, 0 );
}


void set_ioapic_id( uint32_t new_id )
{
	write_ioapic( REG_ID, (new_id<<24) );

	dmesg("Set IOAPIC ID to %i\n", new_id );
}


void init_ioapic()
{
	uint32_t reg;


	if ( read_ioapic( REG_VERSION ) != 0x170011 )
	{
		usingIOAPIC = 0;
		dmesg("IO APIC was not found. Using legacy PIC\n");
		return;
	}

	dmesg("\nIO APIC found\n");

	usingIOAPIC = 1;

	disable_legacy_pic();
	
	reg = REG_INT0;
	
	// Remap all IRQ's to valid interrupts
	while ( reg <= REG_INT23 )
	{
		uint32_t type = (1<<16);	// Masked.

		if ( reg > REG_INT15 )
			type = type | (1<<15) | (1<<13);

	
		// Destination is CPU 0. Set them all masked.
		write_ioapic( reg, type | (0x20 + ((reg - REG_INT0)/2)) );
		write_ioapic( reg+1, 0x0 );	
	
		reg += 2;
	}


	enable_ioapic();
}


