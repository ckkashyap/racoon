#ifndef _KERNEL_ASSERT_H
#define _KERNEL_ASSERT_H

#include "include/dmesg.h"
#include "include/interrupts.h"
#include "include/cpu.h"
#include "include/io.h"

#define assert_build(command)		command

#define assert(condition)			\
		if ( ! (condition) )		\
		{							\
			int i = 0;				\
			dmesg("%!%s(%i): %s(%s), cpu %i\n", __FILE__, __LINE__, "assert condition failed", #condition, CPUID );	\
			dmesg("%!kernel halted. press ESC\n");		\
			sched_lock_all();						\
			while (inb(0x60)!=1) continue;			\
			while (inb(0x60)==1) continue;			\
			i = 10 / i;								\
			while (1==1) { asm ("hlt"); }			\
		}


#define MAGIC_BREAKPOINT		asm("xchgw %bx, %bx")


#endif

