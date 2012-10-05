#pragma once

// STD
#include <stdint.h>


#define MAX_MEM_LEVEL 9

#ifndef MAX_WBITS
#  define MAX_WBITS   15
#endif

typedef unsigned char  Byte;
typedef unsigned int   uInt;
typedef unsigned long  uLong;

typedef Byte		Bytef;
typedef char		charf;
typedef int			intf;
typedef uInt		uIntf;
typedef uLong		uLongf;

typedef void const *voidpc;
typedef void       *voidpf;
typedef void       *voidp;
typedef uint32_t	z_crc_t;
