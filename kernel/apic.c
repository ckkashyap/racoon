#include "include/apic.h"
#include <debug/assert.h>
#include "include/cpu.h"
#include "include/time.h"
#include "include/dmesg.h"


#define TEST_DURATION		200




void load_apic( int cpu_id )
{
	uint32_t after;

	assert( (cpu_id >= 0) && (cpu_id < MAX_CPU) );


	// Software enable the local APIC. Some BIOS's appear to disable it.
	after = APIC_GET( APIC_spurious );
	APIC_SET( APIC_spurious,  ((after | (1<<8)) | (APIC_INTERRUPT + 15)) );

	
	APIC_SET( APIC_LVTtimer, APIC_INTERRUPT );
	APIC_SET( APIC_TIMERdivider, APIC_TIMERdivideBy128 ); 
	
	
	// Starts it counting down..
	APIC_SET( APIC_TIMERinitialCount, 0xFFFFFFFF );
	delay( TEST_DURATION );	// Delay a few ms
	after = 0xFFFFFFFF - APIC_GET( APIC_TIMERcurrentCount );
	APIC_SET( APIC_TIMERinitialCount, 0); 

	cpu[ cpu_id ].busSpeed = (after * (1000 / TEST_DURATION)) / 128;

	APIC_SET( APIC_TIMERdivider, APIC_TIMERdivideBy16 );

	dmesg("CPU %i bus frequency = %i Hz\n", cpu_id, cpu[ cpu_id ].busSpeed );


	APIC_SET( APIC_LVTthermal, (APIC_INTERRUPT + 1) ); 
	APIC_SET( APIC_LVTperfCounter, (APIC_INTERRUPT + 2) ); 
	APIC_SET( APIC_LVT_LINT0, (7<<8) | (APIC_INTERRUPT + 3) ); 
	APIC_SET( APIC_LVT_LINT1, (4<<8) | (APIC_INTERRUPT + 4) ); 
	APIC_SET( APIC_LVTerror, (APIC_INTERRUPT + 5) ); 
	APIC_SET( APIC_taskPriority, 0 );		// Some BIOS leave this quite high
}


void set_apic_distance( int cpu_id, unsigned int milliseconds )
{
	/// \todo Find a way to precalculate...
	assert( milliseconds != 0 );
	assert( (cpu_id >= 0) && (cpu_id < MAX_CPU) );

	uint32_t val;
	uint32_t after = ((cpu[ cpu_id ].busSpeed * 16) / 1000) * milliseconds;
	
	val = APIC_GET((APIC_inServiceRegister + 0x10 * 2));
	assert( ((val >> 16) & 1) == 0 );	// No interrupts pending.


	APIC_SET( APIC_TIMERinitialCount, (uint32_t)after );
}


void ack_apic_timer()
{
	uint32_t val;

	assert( APIC_INTERRUPT == 0x50 );

	APIC_SET( APIC_TIMERinitialCount, 0 );						// Stop it

	// Are we actually servicing an interrupt... ?
	val = APIC_GET((APIC_inServiceRegister + 0x10 * 2));
	if ( ((val >> 16) & 1) != 0 )
	{
		APIC_SET( APIC_endOfInterrupt, 0 );
	}
}

void ack_apic()
{
	APIC_SET( APIC_endOfInterrupt, 0 );
}

void send_interrupt( int cpu_id, int interrupt )
{
	int ei;
	uint32_t* low = (uint32_t*)APIC_lowICR;
	
	assert( (cpu_id >= 0) && (cpu_id < cpu_count) );

	ei = disable_interrupts();
		APIC_SET( APIC_highICR, (((uint32_t)cpu_id)<<24) );
		APIC_SET( APIC_lowICR,  ((1ul<<14)	| interrupt) );
	if ( ei != 0 ) enable_interrupts();

	while ( ((*low) & (1<<12)) != 0 ) continue;

}

// LVT for APIC handling.


void apic_spurious()
{
	dmesg_xy(0,0,"spurious APIC interrupt fired\n");
	APIC_SET( APIC_endOfInterrupt, 0 );
}


void apic_lint0()
{
	dmesg_xy(0,1,"LINT0 APIC interrupt fired.\n");
	APIC_SET( APIC_endOfInterrupt, 0 );
}



void apic_lint1()
{
	dmesg_xy(0,2,"LINT1 APIC interrupt fired\n");
	APIC_SET( APIC_endOfInterrupt, 0 );
}


void apic_error()
{
	dmesg_xy(0,3,"error APIC interrupt fired\n");
	APIC_SET( APIC_endOfInterrupt, 0 );
}


void apic_perfmon()
{
	dmesg_xy(0,4,"perfmon APIC interrupt fired\n");
	APIC_SET( APIC_endOfInterrupt, 0 );
}


void apic_thermal()
{
	dmesg_xy(0,5,"thermal APIC interrupt fired\n");
	APIC_SET( APIC_endOfInterrupt, 0 );
}






