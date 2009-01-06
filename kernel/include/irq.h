#ifndef _KERNEL_IRQ_H
#define _KERNEL_IRQ_H


struct thread;


/** This is the structure which will contain individual
 * IRQ thread information. The irq_hook structure will
 * contain an array of these structs.  */
struct irq_stub
{
	struct thread *tr;			// IRQ handler thread
	unsigned long long ack;		// Number of IRQ's this thread ack'ed
	unsigned long long nop;		// Number of IRQ's this thread rejected
};

/** An IRQ specific structure, 1 per IRQ. This structure
 * will contain a dynamic array of irq_stubs.  */
struct irq_hook
{
	unsigned long long raised;	// Number times this IRQ fired.
	int index;					// IRQ handler currently running.
	int last;					// Last IRQ to handle successfully.
	int count;					// Number of handlers in array.
	struct irq_stub *stubs;		// Stub array.
};



#define IRQ_COUNT	24
#define IRQ_HANDLED	0

void init_irqs();
void load_irqs( int cpu_id );


int request_irq( struct thread *t, int irq );
int release_irq( struct thread *t, int irq );


int irq_ack( int irq );
int irq_complete( int status );

#endif


