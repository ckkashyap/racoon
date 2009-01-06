BASEDIR=$(SOURCE_ROOT)


STATIC_TARGET=libsmk.a
SHARED_TARGET=libsmk.so



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
