#ifndef _LIBKERNEL_INTTYPES_H
#define _LIBKERNEL_INTTYPES_H


#ifndef PAGE_SIZE
#define PAGE_SIZE		4096
#endif

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef _HAVE_SIZE_T
#define _HAVE_SIZE_T
typedef unsigned int size_t;
#endif

#ifndef _HAVE_SSIZE_T
#define _HAVE_SSIZE_T
typedef int ssize_t;
#endif


#ifndef _HAVE_TIME_T
#define _HAVE_TIME_T
typedef unsigned int time_t;
#endif


#ifndef true
#define true 		(1==1)
#endif

#ifndef false
#define false 		(1==0)
#endif


#ifndef _HAVE_UINT64_T
#define _HAVE_UINT64_T
typedef unsigned long long uint64_t;
#endif

#ifndef _HAVE_UINT32_T
#define _HAVE_UINT32_T
typedef unsigned int uint32_t;
#endif

#ifndef _HAVE_UINT16_T
#define _HAVE_UINT16_T
typedef unsigned short uint16_t;
#endif

#ifndef _HAVE_UINT8_T
#define _HAVE_UINT8_T
typedef unsigned char uint8_t;
#endif


#ifndef _HAVE_INT64_T
#define _HAVE_INT64_T
typedef long long int64_t;
#endif

#ifndef _HAVE_INT32_T
#define _HAVE_INT32_T
typedef int int32_t;
#endif

#ifndef _HAVE_INT16_T
#define _HAVE_INT16_T
typedef short int16_t;
#endif

#ifndef _HAVE_INT8_T
#define _HAVE_INT8_T
typedef char int8_t;
#endif


#ifndef _HAVE_INTPTR_T
#define _HAVE_INTPTR_T
typedef int32_t  intptr_t;
#endif

#ifndef _HAVE_UINTPTR_T
#define _HAVE_UINTPTR_T
typedef uint32_t uintptr_t;
#endif



#endif

