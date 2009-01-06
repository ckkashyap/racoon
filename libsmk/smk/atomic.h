#ifndef _LIBKERNEL_ATOMIC_H
#define _LIBKERNEL_ATOMIC_H

#include "inttypes.h"

extern void smk_release_timeslice();

/**  \defgroup ATOMIC  Atomic Operations  
 *
 */


/** @{  */


#ifndef _HAVE_SPINLOCK_T
#define _HAVE_SPINLOCK_T
typedef volatile uint32_t spinlock_t; 
#endif

#define INIT_SPINLOCK	0

static inline void atomic_init( spinlock_t *lock )
{
	*lock = 0;
}

static inline void atomic_add( volatile unsigned int *number, int diff )
{
	asm volatile ( "movl %1, %%eax;\n lock addl %%eax,(%0)\n" 
				: : "p" (number), "g" (diff) : "eax");
}

static inline void atomic_sub( volatile unsigned int *number, int diff )
{
	asm volatile ( "movl %1, %%eax;\n lock subl %%eax, (%0)\n" 
				: : "p" (number), "g" (diff) : "eax" );
}

static inline void atomic_inc( unsigned int *number )
{
  asm volatile ( "lock incl (%0)\n" : : "p" (number) );
}

static inline void atomic_dec( unsigned int *number )
{
  asm volatile ( "lock decl (%0)\n" : : "p" (number) );
}

static inline void atomic_set( int *number, int num )
{
  asm volatile ( "lock xchgl %0, (%1)" : : "g" (num), "p" (number) );
}




static inline void acquire_spinlock( spinlock_t *spinlock )
{
	asm volatile ( "1: lock btsl $0x0, (%0);\n"
					"jnc 2f;\n"
					"pause;\n"
					"jmp 1b;\n"
					"2:\n"
					: : "p" (spinlock) );
}

static inline void release_spinlock( spinlock_t *spinlock )
{
	asm volatile ( "lock btrl $0x0, (%0)\n" : : "p" (spinlock) );
}





static inline void atomic_exchange( void** left, void** memory )
{
  asm volatile ( "movl %1, %%eax\n"
		 "lock xchgl %%eax, (%2)\n" 
		  "movl %%eax, %0"
		  : "=p" (*left) 
		  : "p" (*left), "p" (memory)
		  : "eax" );

}

static inline int atomic_bitset( volatile unsigned int *val, int bit )
{
  char tmp = 0;
	
  asm volatile ( "movl %1, %%eax\n"
		 "movl %2, %%ebx\n"
		 "lock btsl %%ebx, (%%eax)\n"
		 "setc %0\n"
		 : "=g" (tmp)
		 : "p" (val), "g" (bit)
		 : "eax", "ebx"
		);

  if ( tmp == 0 ) return 0;
  return 1;
}

static inline int atomic_bitclear( volatile unsigned int *val, int bit )
{
  char tmp = 0;
	
  asm volatile ( "movl %1, %%eax\n"
		 "movl %2, %%ebx\n"
		 "lock btrl %%ebx, (%%eax)\n"
		 "setc %0\n"
		 : "=g" (tmp)
		 : "p" (val), "g" (bit)
		 : "eax", "ebx"
		);

  if ( tmp == 0 ) return 0;
  return 1;
}





/** @} */


#endif

