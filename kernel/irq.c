#include <smk/strings.h>
#include <smk/atomic.h>

#include <debug/assert.h>
#include <mm/mm.h>
#include "include/misc.h"
#include "include/irq.h"
#include "include/gdt.h"
#include "include/apic.h"
#include "include/cpu.h"
#include "include/interrupts.h"
#include <processes/threads.h>
#include <scheduler/scheduler.h>
#include "include/eflags.h"
#include "include/ioapic.h"

#define PORT_8259M  0x20
#define PORT_8259S  0xA0

/** IRQ hooks & lock */
static spinlock_t		 	irq_locks[IRQ_COUNT];
static struct irq_hook 		*handlers[IRQ_COUNT];

// IRQ mask with cascading allowed.
static uint32_t irq_mask = 0xFFFF - 4;

// COURTESY OF THE OS FAQ WIKI AT www.mega-tokyo.com/forum


void unmask_8259(int irq)
{
  irq_mask = irq_mask & ~( 1ul << irq );

  if ( irq < 8 ) 
  	outb_p( PORT_8259M + 1, (irq_mask & 0xFF) );
  else 
	outb_p( PORT_8259S + 1, (( irq_mask >> 8 ) & 0xFF) );
}

void mask_8259(int irq)
{
  irq_mask = irq_mask | ( 1 << irq );

  if ( irq < 8 ) 
  	   outb_p( PORT_8259M + 1, (irq_mask & 0xFF) );
  else 
  	   outb_p( PORT_8259S + 1, ((irq_mask >> 8) & 0xFF) );
}


void ack_8259( int irq )
{
  if ( irq > 7 )  outb_p(0xA0,0x20);
  outb_p(0x20,0x20); 
}




/* remap the PIC controller interrupts to our vectors
 *    rather than the 8 + 70 as mapped by default */

#define ICW1_ICW4       0x01            /* ICW4 (not) needed */
#define ICW1_SINGLE     0x02            /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04            /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08            /* Level triggered (edge) mode */
#define ICW1_INIT       0x10            /* Initialization - required! */

#define ICW4_8086       0x01            /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02            /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08            /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C            /* Buffered mode/master */
#define ICW4_SFNM       0x10            /* Special fully nested (not) */


int remap_irqs( int pic1, int pic2 )
{
	unsigned char a,b;

	dmesg("Remapping PIC IRQs to %x,%x\n", pic1, pic2 );

	a = inb(PORT_8259M);
	b = inb(PORT_8259S);

		
	    outb_p(PORT_8259M, ICW1_INIT + ICW1_ICW4 );
   		outb_p(PORT_8259S, ICW1_INIT + ICW1_ICW4 );

    	outb_p(PORT_8259M+1, pic1 ); 
    	outb_p(PORT_8259S+1, pic2 ); 
		
    	outb_p(PORT_8259M+1, 0x04);
    	outb_p(PORT_8259S+1, 0x02);

    	outb_p(PORT_8259M+1, ICW4_8086 );
    	outb_p(PORT_8259S+1, ICW4_8086 );

		//outb_p(PORT_8259M, 0xC1 );	// Set priority to 1
		//outb_p(PORT_8259S, 0xC7 );	// Set priority to 7
		
	outb_p(PORT_8259M+1, a);
	outb_p(PORT_8259S+1, b);



	return 0;
}



void init_irqs()
{
	int i;

  	remap_irqs( 0x20, 0x28 );

	// Mask everything to match the initialized value
	outb(PORT_8259M+1, (irq_mask & 0xFF) );
	outb(PORT_8259S+1, (irq_mask >> 8) & 0xFF );

	// NMI
	outb(0x61, inb(0x61) & ~(0xC) );

	for ( i = 0; i < IRQ_COUNT; i++ )
	{
		handlers[i] = (struct irq_hook*)kmalloc( sizeof( struct irq_hook ) );
		handlers[i]->raised = 0;
		handlers[i]->index = 0;
		handlers[i]->last = 0;
		handlers[i]->count = 0;
		handlers[i]->stubs = NULL;
		
		irq_locks[i] = INIT_SPINLOCK;
	}

}



void load_irqs( int cpu_id )
{
	#define CREATE( id, num, function )	\
		extern void function();		\
		set_interrupt( id, num,		\
				(IDT_PRESENT | IDT_32 | IDT_INT), 	\
				GDT_SYSTEMCODE,				\
				(uint32_t)function,			\
				IDT_DPL0	);
		
		CREATE( cpu_id, 0x20, __irq_handler0 );
		CREATE( cpu_id, 0x21, __irq_handler1 );
		CREATE( cpu_id, 0x22, __irq_handler2 );
		CREATE( cpu_id, 0x23, __irq_handler3 );
		CREATE( cpu_id, 0x24, __irq_handler4 );
		CREATE( cpu_id, 0x25, __irq_handler5 );
		CREATE( cpu_id, 0x26, __irq_handler6 );
		CREATE( cpu_id, 0x27, __irq_handler7 );
		CREATE( cpu_id, 0x28, __irq_handler8 );
		CREATE( cpu_id, 0x29, __irq_handler9 );
		CREATE( cpu_id, 0x2A, __irq_handlerA );
		CREATE( cpu_id, 0x2B, __irq_handlerB );
		CREATE( cpu_id, 0x2C, __irq_handlerC );
		CREATE( cpu_id, 0x2D, __irq_handlerD );
		CREATE( cpu_id, 0x2E, __irq_handlerE );
		CREATE( cpu_id, 0x2F, __irq_handlerF );
		CREATE( cpu_id, 0x30, __irq_handler10 );
		CREATE( cpu_id, 0x31, __irq_handler11 );
		CREATE( cpu_id, 0x32, __irq_handler12 );
		CREATE( cpu_id, 0x33, __irq_handler13 );
		CREATE( cpu_id, 0x34, __irq_handler14 );
		CREATE( cpu_id, 0x35, __irq_handler15 );
		CREATE( cpu_id, 0x36, __irq_handler16 );
		CREATE( cpu_id, 0x37, __irq_handler17 );

}



int request_irq( struct thread *t, int irq )
{
	struct irq_hook *hook = NULL;
	
	if ( (irq < 0) || (irq >= IRQ_COUNT) ) return -1;
	if ( t->state != THREAD_SUSPENDED ) return -1;
	
	hook = (struct irq_hook*)kmalloc( sizeof( struct irq_hook ) );
	if ( hook == NULL ) return -1;

	acquire_spinlock( &(irq_locks[irq]) );


		hook->count = handlers[irq]->count;
		hook->stubs = kmalloc( sizeof(struct irq_stub) * (hook->count+1) );
		if ( hook->stubs == NULL )
		{
			release_spinlock( &(irq_locks[irq]) );
			kfree( hook );
			return -1;
		}

		// Copy the old information.
		if ( hook->count != 0 )
		{
			kmemcpy( hook->stubs, 
					handlers[irq]->stubs, 
					sizeof(struct irq_stub) * hook->count );
		}

		hook->stubs[ hook->count ].tr = t;
		hook->stubs[ hook->count ].ack = 0;
		hook->stubs[ hook->count ].nop = 0;

		hook->count  += 1;		// Increase count
		  

		
		sched_lock_all();		// MODIFY THE IRQ INFORMATION
		

			hook->raised = handlers[irq]->raised;
			hook->index  = handlers[irq]->index;
			hook->last   = handlers[irq]->last;

			if ( hook->index < 0 ) hook->index = 0;
			if ( hook->last < 0 )  hook->last = 0;

			if ( hook->count == 1 ) 
			{
				if ( HAS_IOAPIC )
					unmask_ioapic( irq );
				else
					unmask_8259( irq );
				
			}
			
			atomic_exchange( (void**)&hook, (void**)&(handlers[irq]) );

			t->irq = irq;
			t->state = THREAD_IRQ;		// Update thread state

				// Ensure that the thread stays on 1 CPU
			if ( t->cpu_affinity < 0 )
				t->cpu_affinity = ((t->tid - 1) % cpu_count);


		sched_unlock_all();
		

	release_spinlock( &(irq_locks[irq]) );

	assert( hook != NULL );

	if ( hook->stubs != NULL ) kfree( hook->stubs );
	kfree( hook );

	return 0;
}

int release_irq( struct thread *t, int irq )
{
	int i;
	int found = 0;
	struct irq_hook* hook = NULL;

	if ( (irq < 0) || (irq >= IRQ_COUNT) ) return -1;


	hook = (struct irq_hook*)kmalloc( sizeof( struct irq_hook ) );
	if ( hook == NULL ) return -1;
	
	acquire_spinlock( &(irq_locks[irq]) );

	
		hook->count = handlers[irq]->count - 1;
		if ( hook->count == 0 )
		{
			hook->stubs = NULL;

			// Make sure it was the right process.
			if ( handlers[irq]->stubs[0].tr->process == t->process )
					found = 1;
		}
		else
		{
			hook->stubs = kmalloc( sizeof(struct irq_stub) * hook->count );
			if ( hook->stubs == NULL )
			{
				release_spinlock( &(irq_locks[irq]) );
				kfree( hook );
				return -1;
			}

			// Shift everything up ...
			i = 0;
			found = 0;
			while ( i < handlers[irq]->count )
			{
				if ( handlers[irq]->stubs[i].tr == t )
					found += 1;
				else
					hook->stubs[i-found] = handlers[irq]->stubs[i];
	
				i++;
			}
		}
	
		if ( found == 0 )
		{
			release_spinlock( &(irq_locks[irq]) );
			if ( hook->stubs != NULL ) kfree( hook );
			kfree( hook );
			return -1;
		}

		assert( found == 1 );	// Extremely unlikely event otherwise
			
	
		sched_lock_all();		// MODIFY THE IRQ INFORMATION


			hook->raised = handlers[irq]->raised;
			hook->index  = handlers[irq]->index;
			hook->last   = handlers[irq]->last;

			if ( (hook->index >= hook->count) && (hook->index > 0) ) hook->index -= 1;
			if ( (hook->last >= hook->count) && (hook->last > 0) ) hook->last -= -1;

			if ( hook->count == 0 ) 
			{
				if ( HAS_IOAPIC )
					mask_ioapic( irq );
				else
					mask_8259( irq );
			}
			
			atomic_exchange( (void**)&hook, (void**)&(handlers[irq]) );

			t->state = THREAD_SUSPENDED;	// Restore thread.
			t->irq = -1;


		sched_unlock_all();
		

	release_spinlock( &(irq_locks[irq]) );

	assert( hook != NULL );

	if ( hook->stubs != NULL ) kfree( hook->stubs );
	kfree( hook );
	return 0;
}


/** Purely acknowledge the interrupt */
int irq_ack( int irq )
{
	struct thread *tr = current_thread();
	int pos;
	int cl;

	if ( (irq < 0) || (irq >= IRQ_COUNT) ) return -1;
		
	acquire_spinlock( &(irq_locks[irq]) );


		pos = (handlers[irq]->index + handlers[irq]->last) % handlers[irq]->count;

		if ( handlers[irq]->stubs[ pos ].tr != tr )
		{
			dmesg("%!IRQ LIES\n");
			release_spinlock( &(irq_locks[irq]) );
			return -1;
		}

	cl = disable_interrupts();

		if ( HAS_IOAPIC )
		{
//			unmask_ioapic( irq );
			ack_apic();
		}
		else
		{
//			unmask_8259( irq );
			ack_8259( irq );
		}


		release_spinlock( &(irq_locks[irq]) );

	if ( cl != 0 ) enable_interrupts();
	return 0;
}



int irq_complete( int status )
{
	struct thread *tr = current_thread();
	int irq = tr->irq;
	int pos;
	int rc;
	int cl;

	if ( (irq < 0) || (irq >= IRQ_COUNT) ) return -1;
		
	acquire_spinlock( &(irq_locks[irq]) );


		pos = (handlers[irq]->index + handlers[irq]->last) % handlers[irq]->count;

		// Lies!
		if ( handlers[irq]->stubs[ pos ].tr != tr )
		{
			dmesg("%!IRQ LIES\n");
			release_spinlock( &(irq_locks[irq]) );
			return -1;
		}

	
		if ( status == 0 )
		{
			handlers[irq]->stubs[ pos ].ack += 1;
			handlers[irq]->last = pos;
			rc = 0;
		}
		else
		{
			handlers[irq]->stubs[ pos ].nop += 1;
			handlers[irq]->index += 1;

			if ( handlers[irq]->index < handlers[irq]->count )
			{
				pos = (pos + 1) % handlers[irq]->count;
				fast_queue( handlers[irq]->stubs[pos].tr );
				rc = 1;
			}
			else
			{
				rc = 2;
			}
		}


	cl = disable_interrupts();
	assert( cl != 0 );


		fast_dequeue( tr );
	
		release_spinlock( &(irq_locks[irq]) );

	if ( rc == 0 ) sched_yield();

	enable_interrupts();
	return 0;
}



// This is a debug condition which requires that nothing else
// can be occuring whilst we're inside an IRQ handling procedure.
assert_build( volatile int INSIDE_IRQ = 0; )


/** All IRQ's which occur in the system trigger this. */
int irq_handler( int irq )
{
	assert( (cpu_flags() & EFLAG_INTERRUPT) == 0 );

	/*
	((char*)0xB8000)[ 160 * 24 + (158-IRQ_COUNT*2) + irq * 2 ]++;
	((char*)0xB8000)[ 160 * 24 + (156-IRQ_COUNT*2) ] = '|';
	*/


	if ( irq == 0 )
	{
		if ( HAS_IOAPIC ) ack_apic();
			 	 	 else ack_8259( irq );

		return 0;
	}


	assert_build( INSIDE_IRQ = 1; )

	if ( HAS_IOAPIC ) 
	{
//		ack_apic();
//		mask_ioapic( irq );			// Mask this one
	}
	else
	{
//		ack_8259( irq );
//	 	mask_8259( irq );
	}


		handlers[irq]->raised += 1;			// Accounting
		handlers[irq]->index   = 0;			// Dynamic list reset


		if ( handlers[irq]->count <= 0 )
		{
			dmesg("%!Spurios, unexpected IRQ %i\n", irq );
			assert_build( INSIDE_IRQ = 0; );
			return 0;
		}

		assert( handlers[irq]->count > 0 );		// Should never occur..

		
								// Queue the handler
		fast_queue( handlers[irq]->stubs[ handlers[irq]->last ].tr );

	assert_build( INSIDE_IRQ = 0; )

	return 0;
}





