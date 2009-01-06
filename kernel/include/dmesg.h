#ifndef _KERNEL_DMESG_H
#define _KERNEL_DMESG_H

void dmesg_clear();
int dmesg( char *format, ... );
int dmesg_xy( int x, int y, char *format, ... );

#endif

