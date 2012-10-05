#ifndef ZUTIL_H
#define ZUTIL_H

#include "zlib.h"

#if defined(STDC) && !defined(Z_SOLO)
#  if !(defined(_WIN32_WCE) && defined(_MSC_VER))
#    include <stddef.h>
#  endif
#  include <string.h>
#  include <stdlib.h>
#endif

#ifdef Z_SOLO
   typedef long ptrdiff_t;  
#endif

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

#if defined(MACOS) || defined(TARGET_OS_MAC)
#  define OS_CODE  0x07
#  ifndef Z_SOLO
#    if defined(__MWERKS__) && __dest_os != __be_os && __dest_os != __win32_os
#      include <unix.h> 
#    else
#      ifndef fdopen
#        define fdopen(fd,mode) NULL 
#      endif
#    endif
#  endif
#endif

#ifdef WIN32
#  ifndef __CYGWIN__  
#    define OS_CODE  0x0b
#  endif
#endif

#if (defined(_MSC_VER) && (_MSC_VER > 600)) && !defined __INTERIX
#  if defined(_WIN32_WCE)
#    define fdopen(fd,mode) NULL 
#    ifndef _PTRDIFF_T_DEFINED
       typedef int ptrdiff_t;
#      define _PTRDIFF_T_DEFINED
#    endif
#  else
#    define fdopen(fd,type)  _fdopen(fd,type)
#  endif
#endif
      

#ifndef OS_CODE
#  define OS_CODE  0x03  
#endif

#ifndef F_OPEN
#  define F_OPEN(name, mode) fopen((name), (mode))
#endif

         

#if defined(pyr) || defined(Z_SOLO)
#  define NO_MEMCPY
#endif
#if defined(SMALL_MEDIUM) && !defined(_MSC_VER) && !defined(__SC__)
 
#  define NO_MEMCPY
#endif
#if defined(STDC) && !defined(HAVE_MEMCPY) && !defined(NO_MEMCPY)
#  define HAVE_MEMCPY
#endif
#ifdef HAVE_MEMCPY
#  ifdef SMALL_MEDIUM 
#    define zmemcpy _fmemcpy
#    define zmemcmp _fmemcmp
#    define zmemzero(dest, len) _fmemset(dest, 0, len)
#  else
#    define zmemcpy memcpy
#    define zmemcmp memcmp
#    define zmemzero(dest, len) memset(dest, 0, len)
#  endif
#else
   void zmemcpy OF((Bytef* dest, const Bytef* source, uInt len));
   int zmemcmp OF((const Bytef* s1, const Bytef* s2, uInt len));
   void zmemzero OF((Bytef* dest, uInt len));
#endif

#ifndef Z_SOLO
   voidpf zcalloc OF((voidpf opaque, unsigned items,
                                    unsigned size));
   void zcfree  OF((voidpf opaque, voidpf ptr));
#endif

#define ZALLOC(strm, items, size) \
           (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr)  (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))
#define TRY_FREE(s, p) {if (p) ZFREE(s, p);}


#define ZSWAP32(q) ((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
                    (((q) & 0xff00) << 8) + (((q) & 0xff) << 24))

#endif 
