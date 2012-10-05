#pragma once


#if defined(_WINDOWS) && !defined(WINDOWS)
#  define WINDOWS
#endif
#if defined(_WIN32) || defined(__WIN32__)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#if (defined(MSDOS) || defined(OS2) || defined(WINDOWS)) && !defined(WIN32)
#  if !defined(__GNUC__) && !defined(__FLAT__) && !defined(__386__)
#    ifndef SYS16BIT
#      define SYS16BIT
#    endif
#  endif
#endif

#ifdef __STDC_VERSION__
#  ifndef STDC
#    define STDC
#  endif
#  if __STDC_VERSION__ >= 199901L
#    ifndef STDC99
#      define STDC99
#    endif
#  endif
#endif
#if !defined(STDC) && (defined(__STDC__) || defined(__cplusplus))
#  define STDC
#endif
#if !defined(STDC) && (defined(__GNUC__) || defined(__BORLANDC__))
#  define STDC
#endif
#if !defined(STDC) && (defined(MSDOS) || defined(WINDOWS) || defined(WIN32))
#  define STDC
#endif
#if !defined(STDC) && (defined(OS2) || defined(__HOS_AIX__))
#  define STDC
#endif

#if defined(__OS400__) && !defined(STDC)
#  define STDC
#endif

#ifndef STDC
#  ifndef const
#    define const
#  endif
#endif

#if defined(ZLIB_CONST) && !defined(z_const)
#  define z_const const
#else
#  define z_const
#endif

#ifndef MAX_MEM_LEVEL
#  ifdef MAXSEG_64K
#    define MAX_MEM_LEVEL 8
#  else
#    define MAX_MEM_LEVEL 9
#  endif
#endif

#ifndef MAX_WBITS
#  define MAX_WBITS   15
#endif

#ifndef OF
#  ifdef STDC
#    define OF(args)  args
#  else
#    define OF(args)  ()
#  endif
#endif

#ifndef Z_ARG
#  if defined(STDC) || defined(Z_HAVE_STDARG_H)
#    define Z_ARG(args)  args
#  else
#    define Z_ARG(args)  ()
#  endif
#endif

#ifndef ZEXTERN
#  define ZEXTERN extern
#endif

#if !defined(__MACTYPES__)
typedef unsigned char  Byte;
#endif
typedef unsigned int   uInt;
typedef unsigned long  uLong;

#ifdef SMALL_MEDIUM

#  define Bytef Byte
#else
   typedef Byte Bytef;
#endif
typedef char  charf;
typedef int   intf;
typedef uInt  uIntf;
typedef uLong uLongf;

#ifdef STDC
   typedef void const *voidpc;
   typedef void       *voidpf;
   typedef void       *voidp;
#else
   typedef Byte const *voidpc;
   typedef Byte       *voidpf;
   typedef Byte       *voidp;
#endif

#ifdef Z_U4
   typedef Z_U4 z_crc_t;
#else
   typedef unsigned long z_crc_t;
#endif

#ifdef HAVE_UNISTD_H
#  define Z_HAVE_UNISTD_H
#endif

#ifdef HAVE_STDARG_H
#  define Z_HAVE_STDARG_H
#endif

#ifdef STDC
#    include <sys/types.h>
#endif

#ifdef _WIN32
#  include <stddef.h>
#endif

#ifndef z_off_t
#  define z_off_t long
#endif

#if !defined(_WIN32) && defined(Z_LARGE64)
#  define z_off64_t off64_t
#else
#    define z_off64_t z_off_t
#endif
