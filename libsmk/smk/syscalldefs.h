#ifndef _LIBKERNEL_SYSCALLDEFS_H
#define _LIBKERNEL_SYSCALLDEFS_H


#define     SYS_MEMORY      (1U<<8)
#define		SYS_SEMAPHORE	(2U<<8)
#define		SYS_SYSTEM		(3U<<8)
#define		SYS_PROCESS		(4U<<8)
#define		SYS_THREAD		(5U<<8)
#define		SYS_CAP			(6U<<8)
#define 	SYS_PULSE		(7U<<8)
#define 	SYS_MESSAGE		(8U<<8)
#define 	SYS_PORT		(9U<<8)
#define     SYS_EXEC		(10U<<8)
#define		SYS_PCI			(11U<<8)
#define		SYS_TIME		(12U<<8)
#define		SYS_IRQ			(13U<<8)
#define 	SYS_IO			(14U<<8)
#define		SYS_CONSOLE		(15U<<8)
#define		SYS_LFB			(16U<<8)
#define 	SYS_INFO		(17U<<8)
#define     SYS_MISC		(18U<<8)
#define     SYS_SHMEM		(19U<<8)
#define     SYS_EVENTS		(20U<<8)
#define     SYS_ENV			(21U<<8)

// System call functions

#define		SYS_ONE			1U
#define		SYS_TWO			2U
#define		SYS_THREE		3U
#define		SYS_FOUR		4U
#define		SYS_FIVE		5U
#define		SYS_SIX			6U
#define		SYS_SEVEN		7U
#define		SYS_EIGHT		8U
#define		SYS_NINE		9U
#define		SYS_TEN			10U


#define	OP( major, minor )	(SYS_##major|SYS_##minor)

#define	SYSCALL_OP(num)		(num & 0xFF00)
#define	SYSCALL_FUNC(num)	(num & 0x00FF)


#endif


