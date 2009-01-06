#include <smk.h>

		
/**  \defgroup SHMEM  Shared Memory  


<P>
Shared memory is a section of physical memory which 
is mapped into more than one processes address space.  
The regions may not have the same virtual address in 
any process but any write by one process will immediately 
appear in the other processes memory map (Because it's 
the same piece of memory.).



<H4>Example: Using both Pulses and Shared Memory to 
communicate a large message</H4>

<P>
A process can share a section of memory with B process. A then 
sends a pulse to B with the remote shared memory id. B receives 
the id, accepts the memory and then reads the huge section that 
A process shared with it.  Once B is done with the data, it 
releases the shared memory.

  
*/
		  

/** @{ */


int smk_create_shmem( const unsigned char *name, 
						int pages, 
						unsigned int flags )
{
	return _sysenter( (SYS_SHMEM|SYS_ONE),
					  (uint32_t)name, 
					  (uint32_t)pages, flags, 0,0 );
}


int smk_create_shmem_direct( const unsigned char *name, 
							 int pages, 
						 	 unsigned int flags,
							 uintptr_t location
							 )
{
	return _sysenter( (SYS_SHMEM|SYS_ONE),
					  (uint32_t)name, 
					  (uint32_t)pages, 
					  flags, 
					  1,
					  (uint32_t)location );
}



int smk_grant_shmem( int id, int pid, unsigned int flags )
{
	return _sysenter( (SYS_SHMEM|SYS_THREE),
					  0,
					  (uint32_t)id,
					  (uint32_t)pid,
					  (uint32_t)flags,
					  0 );
}


int smk_revoke_shmem( int id, int pid )
{
	return _sysenter( (SYS_SHMEM|SYS_THREE),
					  1,
					  (uint32_t)id,
					  (uint32_t)pid,
					  0,
					  0 );
}


int smk_request_shmem( int id, void **location, 
						int *pages, unsigned int *flags )
{
	return _sysenter( (SYS_SHMEM|SYS_FOUR),
						0,
						(uint32_t)id,
						(uint32_t)location,
						(uint32_t)pages,
						(uint32_t)flags );
}

int smk_release_shmem( int id )
{
	return _sysenter( (SYS_SHMEM|SYS_FOUR), 1, (uint32_t)id, 0, 0, 0 );
}

int smk_delete_shmem( int id )
{
	return _sysenter( (SYS_SHMEM|SYS_TWO), (uint32_t)id, 0, 0,0,0 );
}

int smk_find_shmem( const unsigned char *name, int *pid )
{
	return _sysenter( (SYS_SHMEM|SYS_FIVE), 
						(uint32_t)name, (uint32_t)pid,0,0,0 );
}


int smk_get_shmem_info( int id, unsigned char *name,
							int *pid, int *pages, 
							unsigned int *flags )
{
	return _sysenter( (SYS_SHMEM|SYS_SIX), 
						(uint32_t)id,
						(uint32_t)name, 
						(uint32_t)pid, 
						(uint32_t)pages,
						(uint32_t)flags );
}



/** @} */



