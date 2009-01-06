#include <smk.h>

/** \addtogroup TIME
 * 
 * @{ 
 *
 */


/** This system call will return the current system seconds and milliseconds
 * since the epoch.
 *
 * \return 0 on successful return.
 */
int smk_system_time( unsigned int *seconds, unsigned int *milliseconds)
{
	return _sysenter( (SYS_TIME|SYS_TWO), 
						(unsigned int)seconds, 
						(unsigned int)milliseconds, 
						0,
						0,
						0 );
}



void smk_ndelay( unsigned int milliseconds )
{
  int d0;
  __asm__ __volatile__(
    "\tjmp 1f\n"
    ".align 16\n"
    "1:\tjmp 2f\n"
    ".align 16\n"
    "2:\tdecl %0\n\tjns 2b"
    :"=&a" (d0)
    :"0" (milliseconds));
}



/** This releases the timeslice of the application by calling the
 * APIC interrupt. This triggers the scheduler which then lets
 * another thread run if any is waiting.
 */
void smk_release_timeslice()
{
	asm volatile ( "int $0x50" );
}


/**  @}  */

