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

extern const char * const z_errmsg[10]; 


#define ERR_MSG(err) z_errmsg[Z_NEED_DICT-(err)]

#define ERR_RETURN(strm,err) \
  return (strm->msg = (char*)ERR_MSG(err), (err))


        

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

        

#if defined(MSDOS) || (defined(WINDOWS) && !defined(WIN32))
#  define OS_CODE  0x00
#  ifndef Z_SOLO
#    if defined(__TURBOC__) || defined(__BORLANDC__)
#      if (__STDC__ == 1) && (defined(__LARGE__) || defined(__COMPACT__))
         
         void _Cdecl farfree( void *block );
         void *_Cdecl farmalloc( unsigned long nbytes );
#      else
#        include <alloc.h>
#      endif
#    else 
#      include <malloc.h>
#    endif
#  endif
#endif

#ifdef AMIGA
#  define OS_CODE  0x01
#endif

#if defined(VAXC) || defined(VMS)
#  define OS_CODE  0x02
#  define F_OPEN(name, mode) \
     fopen((name), (mode), "mbc=60", "ctx=stm", "rfm=fix", "mrs=512")
#endif

#if defined(ATARI) || defined(atarist)
#  define OS_CODE  0x05
#endif

#ifdef OS2
#  define OS_CODE  0x06
#  if defined(M_I86) && !defined(Z_SOLO)
#    include <malloc.h>
#  endif
#endif

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

#ifdef TOPS20
#  define OS_CODE  0x0a
#endif

#ifdef WIN32
#  ifndef __CYGWIN__  
#    define OS_CODE  0x0b
#  endif
#endif

#ifdef __50SERIES 
#  define OS_CODE  0x0f
#endif

#if defined(_BEOS_) || defined(RISCOS)
#  define fdopen(fd,mode) NULL 
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

#if defined(__BORLANDC__) && !defined(MSDOS)
  #pragma warn -8004
  #pragma warn -8008
  #pragma warn -8066
#endif


#if !defined(_WIN32) && (!defined(_LARGEFILE64_SOURCE) || _LFS64_LARGEFILE-0 == 0)
    ZEXTERN uLong ZEXPORT adler32_combine64 OF((uLong, uLong, z_off_t));
    ZEXTERN uLong ZEXPORT crc32_combine64 OF((uLong, uLong, z_off_t));
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

#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)

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
