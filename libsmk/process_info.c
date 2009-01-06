#include <smk/process_info.h>



const char*	smk_process_state_str( int state )
{
	switch (state)
	{
		case PROCESS_INIT:		return "init";
		case PROCESS_RUNNING:	return "running";
		case PROCESS_STOPPED:	return "stopped";
		case PROCESS_FINISHING:	return "finishing";
		case PROCESS_FINISHED:	return "finished";
		case PROCESS_CRASHING:	return "crashing";
		case PROCESS_CRASHED:	return "crashed";
	}
	
	return "unknown";
}



