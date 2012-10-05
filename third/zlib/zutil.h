#ifndef ZUTIL_H
#define ZUTIL_H

#include <stdlib.h>
#include <string.h>
#include "zlib.h"

#ifndef local
#  define local static
#endif

typedef unsigned char  uch;
typedef uch FAR uchf;
typedef unsigned short ush;
typedef ush FAR ushf;
typedef unsigned long  ulg;

#ifndef DEF_WBITS
#  define DEF_WBITS MAX_WBITS
#endif


#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif


#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2


#define MIN_MATCH  3
#define MAX_MATCH  258

#define PRESET_DICT 0x20 


#ifndef F_OPEN
#  define F_OPEN(name, mode) fopen((name), (mode))
#endif

#define zmemzero(dest, len) memset(dest, 0, len)

#define ZALLOC(items, size) malloc( (items) * (size))
#define ZFREE(addr)  free((voidpf)(addr))

#define ZSWAP32(q) ((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
                    (((q) & 0xff00) << 8) + (((q) & 0xff) << 24))

#endif 
