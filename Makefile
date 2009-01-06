all:
	make SOURCE_ROOT=$$PWD -C userlandinit
	make SOURCE_ROOT=$$PWD -C libsmk
	make SOURCE_ROOT=$$PWD -C kernel

clean:
	make SOURCE_ROOT=$$PWD -C userlandinit clean
	make SOURCE_ROOT=$$PWD -C libsmk clean
	make SOURCE_ROOT=$$PWD -C kernel clean
