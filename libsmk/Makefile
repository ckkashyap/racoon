BASEDIR=../../


STATIC_TARGET=libsmk.a
SHARED_TARGET=libsmk.so


OBJS=	env.o				\
		events.o			\
		exec.o				\
		io.o				\
		irq.o				\
		liballoc.o			\
		liballoc_hooks.o	\
		messages.o			\
		misc.o				\
		pci.o				\
		ports.o				\
		process.o			\
		pulse.o				\
		shmem.o				\
		start.o				\
		threads.o			\
		time.o				\
		irq_asm.o			\
		threads_asm.o	


HDRS= 	smk/io.h		\
		smk/threads.h	\
		smk/pci.h		\
		smk/env.h		\
		smk/irq.h		\
		smk/ports.h		\
		smk/pulse.h		\
		smk/messages.h	\
		smk/process.h	\
		smk/misc.h		\
		smk/atomic.h	\
		smk/time.h		\
		smk/events.h	\
		smk/exec.h		\
		smk/shmem.h		\
		smk/alloc.h		\
		smk.h			

OBJS = 	crt0.o		\
		env.o		\
		start.o		\
		strings.o	\
		sysenter.o	\
		process_info.o		\
		system.o	\
		mem.o

HDRS = 	smk/atomic.h	\
		smk/mem.h		\
	  	smk/inttypes.h	\
		smk/limits.h	\
		smk/strings.h	\
		smk/err.h		\
		smk/shared.h	\
		smk/sysenter.h	\
		smk/syscalls.h	\
		smk/syscalldefs.h	\
		smk/system.h	\
		smk/systemdefs.h	\
		smk/process_info.h	\
		smk/kernel_info.h	\
		smk/varargs.h

		

include $(BASEDIR)/glue/make.lib
