#include <smk/inttypes.h>
#include <smk/limits.h>
#include <smk/process_info.h>
#include <smk/strings.h>

#include <debug/assert.h>
#include <mm/multiboot.h>
#include "include/misc.h"

#include <mm/physmem.h>

#include "include/exec.h"


#include "include/smk.h"




static int build_module( const char *command_line, 
							uintptr_t start, uintptr_t finish )
{
	int k, j = 0;
	char name[SMK_NAME_LENGTH];

	for ( k = 0; k < kstrlen( command_line ); k++ )
	{
		if ( command_line[k] == '/' ) j = k+1;
		if ( command_line[k] == ' ' ) break;
	}
	
	k = k - j;
	if ( k >= SMK_NAME_LENGTH ) k = SMK_NAME_LENGTH - 1;

	kstrncpy( name, (command_line + j), k );
	name[k] = 0;
  
	if ( exec_memory_region( smk_process->pid, name, 
								start, finish, command_line ) <= 0 )
			return -1;

	return 0;
}




int init_modules( multiboot_info_t *mboot )
{
	int i;

	for (i = 0; i < mboot->mods_count; i++)
	{
		module_t *mod = (module_t*)( mboot->mods_addr + i * sizeof(module_t) );


		if ( build_module( (const char*)mod->string, mod->mod_start, mod->mod_end ) != 0 )
		{
			dmesg("%!Unknown format for module: %s\n", (const char*)mod->string  );
			return -1;
		}
	}    

	return 0;
}


#include <scheduler/scheduler.h>

int start_first_module()
{
	int ans = 0;
	struct process *proc = checkout_process( 2, READER );
	if ( proc == NULL ) return -1;
	
	struct thread *tr = get_thread( proc, 1 ); 
	if ( tr == NULL )
	{
		commit_process( proc );
		return -1;
	}

	proc->state = PROCESS_RUNNING;
	set_thread_state( tr, THREAD_RUNNING );

	commit_process( proc );
	return ans;
}




