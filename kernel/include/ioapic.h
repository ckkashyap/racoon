#ifndef _KERNEL_IOAPIC_H
#define _KERNEL_IOAPIC_H

#include <smk/inttypes.h>


extern int usingIOAPIC;

#define HAS_IOAPIC	(usingIOAPIC == 1)

void set_ioapic_id( uint32_t new_id );

void mask_ioapic( int num );
void unmask_ioapic( int num );
void init_ioapic();

#endif

