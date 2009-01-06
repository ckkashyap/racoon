#ifndef _KERNEL_SMK_H
#define _KERNEL_SMK_H

#include <processes/process.h>
#include <processes/threads.h>
#include "cpu.h"

extern struct process *smk_process;
extern struct thread  *smk_idle[MAX_CPU];
extern struct thread  *smk_gc[MAX_CPU];


void init_smk();

void load_smk( int cpu_id );

#endif

