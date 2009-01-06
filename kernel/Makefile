BASEDIR=$(SOURCE_ROOT)
include $(BASEDIR)/glue/make.system

KERNEL_VERSION=snapshot
KERNEL_BIN=kernel



FILES = *.c ds/*.c ipc/*.c mm/*.c scheduler/*.c processes/*.c *.S syscalls/*.c

LINKER_SCRIPT=-Xlinker --script=kernel.ld 

INCLUDES=-I. -I$(INCLUDEROOT) 

LIBRARIES= $(LIBRARYROOT)/libsmk.a


CFLAGS= -O0 -static -Wall -nostartfiles -nostdlib 	\
		-nodefaultlibs -nostdinc -fno-builtin   	\
		-DKERNEL_VERSION="\"$(KERNEL_VERSION)\""	\
		-D_KERNEL_

CDEBUGFLAGS= -O0 -static -Wall -nostartfiles -nostdlib 		\
			 -nodefaultlibs -nostdinc -fno-builtin   		\
			 -DKERNEL_VERSION="\"$(KERNEL_VERSION)\""		\
			 -D_KERNEL_ -fno-inline -g


all: $(KERNEL_BIN)


$(KERNEL_BIN): $(FILES)
	$(CC) $(INCLUDES) $(CFLAGS) $(FILES) $(LIBRARIES) $(LINKER_SCRIPT) -o $(KERNEL_BIN)

debug: CFLAGS=$(CDEBUGFLAGS)
debug: $(KERNEL_BIN)


clean:
	$(RM) $(KERNEL_BIN)
#	$(RM) syscalls/*.c

realclean: clean


install: $(KERNEL_BIN)
	$(INSTALL) $(KERNEL_BIN) $(SYSTEMROOT)/$(KERNEL_BIN); 


uninstall:
	$(RM) $(SYSTEMROOT)/$(KERNEL_BIN);


test:
	cd tests && ./runtests.pl


