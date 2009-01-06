#include <smk/err.h>
#include <smk/strings.h>
#include <mm/mm.h>

#include "include/env.h"
#include <processes/process.h>
#include "include/misc.h"


///   \todo Evaluate whether or not I can switch to an htable here.



/** This is a structure used to store environment information for
 * the process.  
 */
struct environ_info
{
	char name[SMK_ENVNAME_LENGTH];
	size_t size;
	void *data;
};




/** Increases the process' environment memory and adds the information to the
 * last position.
 *
 * \warning Currently loses all environment variables if system runs
 * out of memory, due to realloc retuning NULL on failure.
 * 
 * \param proc A W-LOCKED process or dead process, please.
 * \param name The environment name which will identify the data.
 * \param data The actual data to store.
 * \param size The number of bytes in data.
 * \return 0 on successful addition. 
 */ 

int	set_environment( struct process *proc, const char name[SMK_ENVNAME_LENGTH], const void *data, size_t size )
{
	struct environ_info *env = (struct environ_info *)proc->environment;
   	int pos = proc->environment_count;
	int new_size = (pos + 1)  *  sizeof( struct environ_info );
	void *tmp;
	int i = 0;

	// Sanity check.
	if ( data == NULL ) return SMK_BAD_PARAMS;
	if ( name == NULL ) return SMK_BAD_PARAMS;
	if ( size == 0 ) return SMK_REQUESTED_NOTHING;

	// Overwrite if already there.
	for ( i = 0; i < proc->environment_count; i++ )
	{
		if ( kstrncmp( env[i].name, name, SMK_ENVNAME_LENGTH ) == 0 ) 
		{
			void *tmp = env[i].data;

			env[i].data = kmalloc( size );
			if ( env[i].data == NULL )
			{
				env[i].data = tmp;
				return SMK_NOMEM;
			}

			kfree( tmp );
			env[i].size = size;
			kmemcpy( env[i].data, data, size );
			return SMK_OK;
		}
	}

	
	// Try and allocate memory for it first,
	tmp = kmalloc( size );
	if ( tmp == NULL ) return SMK_NOMEM;
	
		// Resize the location.
		proc->environment = krealloc( proc->environment, new_size );
		if ( proc->environment == NULL )
		{
			// realloc failed!
			proc->environment_count = 0;
			kfree( tmp );
			return SMK_NOMEM;
		}
	
	// Copy the data in.
	kmemcpy( tmp, data, size ); 
	
	// Set the environment up.
	env = (struct environ_info *)proc->environment;

	kstrncpy( env[ pos ].name, name, SMK_ENVNAME_LENGTH );  
	env[pos].data = tmp;
	env[pos].size = size;

	// Increase count.
	proc->environment_count += 1;
	return SMK_OK;
}


/** Returns the environment data which is identified with the given 
 * name. The size variable is given to limit the amount of information
 * returned in data.
 *
 *
 * \param proc A R-LOCKED process or dead process, please.
 * \param name The name of the environment variable to return.
 * \param data The buffer to copy the stored variable into.
 * \param size The largest amount of data to return in data.
 * \return the amount of bytes returned in data.
 */

int get_environment( struct process *proc, const char name[SMK_ENVNAME_LENGTH], void *data, size_t *size )
{
	struct environ_info *env = (struct environ_info *)proc->environment;
	size_t new_size;
	int i;

	for ( i = 0; i < proc->environment_count; i++ )
	{
		if ( kstrncmp( env[i].name, name, SMK_ENVNAME_LENGTH ) == 0 ) 
		{
			new_size = env[i].size;
			if ( new_size > *size ) new_size = *size;

			kmemcpy( data, (env[i].data), new_size );
			*size = new_size;
			return SMK_OK;
		}
	}

	return SMK_NOT_FOUND;
}



/** Returns the size of the environment variable with the given 
 * name,
 *
 * \param proc A R-LOCKED process or dead process, please.
 * \param name The name of the environment variable.
 * \param size The size of the data.
 * \return 0 on successful retrieval.
 */
int get_environment_size( struct process* proc, const char name[SMK_ENVNAME_LENGTH], size_t *size )
{
	struct environ_info *env = (struct environ_info *)proc->environment;
	int i;

	for ( i = 0; i < proc->environment_count; i++ )
	{
		if ( kstrncmp( env[i].name, name, SMK_ENVNAME_LENGTH ) == 0 ) 
		{
			*size = env[i].size;
			return 0;
		}
	}

	return SMK_NOT_FOUND;
}



/** Returns the name and size of the environment variable at the
 * given position in the environment list.
 *
 * 
 * \param proc A R-LOCKED process or dead process, please.
 * \param i the position of the environment variable.
 * \param name The name of the environment variable.
 * \param size The size of the data.
 * \return 0 on successful retrieval.
 */
int get_environment_information( struct process* proc, int id, char name[SMK_ENVNAME_LENGTH], size_t *size )
{
    struct environ_info *env = (struct environ_info *)proc->environment;
	
	if ( id < 0 ) return SMK_NOT_FOUND;
	if ( id >= proc->environment_count ) return SMK_NOT_FOUND;

	kstrncpy( name, env[id].name, SMK_ENVNAME_LENGTH );
	*size = env[id].size;

	return SMK_OK;
}


/** Removes a particular environment variable from the process'
 * environment list and kfrees all associated data.
 *
 * \param proc A W-LOCKED process or dead process, please.
 * \param name The name of the environment variable to remove.
 * \return 0 on successful deletion.
 */
int remove_environment( struct process *proc, const char name[SMK_ENVNAME_LENGTH] )
{
	struct environ_info *env = (struct environ_info *)proc->environment;
	int found = 0;
	int i;
	int new_size;

	for ( i = 0; i < proc->environment_count; i++ )
	{
		if ( found == 0 )
		{
			if ( kstrncmp( env[i].name, name, SMK_ENVNAME_LENGTH ) == 0 ) 
			{
				// Okay, found it.
				kfree( env[i].data );	// Free it.
				found = 1;				// Note it.
			}
		}
		else
		{
			// We are just correcting at the moment. Will never run on i==0.
			env[i - 1].data = env[i].data;
			env[i - 1].size = env[i].size;
			kmemcpy( env[i -1].name, env[i].name, SMK_ENVNAME_LENGTH );
		}
		
	}

	if ( found == 0 ) return SMK_NOT_FOUND;	// No good.
	
	new_size = (proc->environment_count - 1)  *  sizeof( struct environ_info );

	// Safe for new_size == 0
	proc->environment = krealloc( proc->environment, new_size );
	proc->environment_count -= 1;
	
	return SMK_OK;
}


/** Frees all environment information belonging to a process and
 * resets the process variables. So it's like it's fresh all
 * over.
 *
 * \param proc A W-LOCKED for writing or dead process please.
 * \return 0 on success.
 */
int release_environment( struct process *proc )
{
	int i;
	struct environ_info *env = (struct environ_info *)proc->environment;
		
	// Only if there's work.
	if ( env != NULL )
	{
		for ( i = 0; i < proc->environment_count; i++ )
			kfree( env[i].data );

		kfree( env );
	}
	
	proc->environment = NULL;
	proc->environment_count = 0;
	return SMK_OK;
}



/** This function takes the environment variables set up in the
 * source process and duplicates (copies) everything across to the
 * destination process.
 *
 * \warning This function first calls release_environment on the
 * destination process. Also, if an error occurs during the clone
 * then the destination process is left without any environment
 * variables.
 *
 * \param src The R-LOCKED source process to get environment variables from.
 * \param dest The W-LOCKED dest process to copy environment variables into.
 * \return 0 on success.
 */
int clone_environment( struct process *dest, struct process *src )
{
	int i;
	struct environ_info *env = (struct environ_info *)src->environment;

	if ( release_environment( dest ) != 0 ) return -1;
	
	if ( env == NULL ) return 0; 	// Already done. :)
		

	for ( i = 0; i < src->environment_count; i++ )
	{
		if ( set_environment( dest, env[i].name,
							   		env[i].data,
									env[i].size ) != 0 )
		{
			release_environment( dest );
			return -1;
		}	
	}

	
	return 0;
}




