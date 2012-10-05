#pragma once

// STD
#include <stdint.h>

#define ZLIB_VERNUM 0x1270
#define ZLIB_VER_MAJOR 1
#define ZLIB_VER_MINOR 2
#define ZLIB_VER_REVISION 7
#define ZLIB_VER_SUBREVISION 0

struct internal_state;

typedef unsigned char  Byte;
typedef unsigned int   uInt;
typedef unsigned long  uLong;

typedef uInt		uIntf;
typedef uLong		uLongf;

typedef void const *voidpc;
typedef void       *voidpf;
typedef void       *voidp;
typedef uint32_t	z_crc_t;

typedef struct z_stream_s {
    const Byte *next_in;
    uInt     avail_in;

    Byte    *next_out;
    uInt     avail_out;

    struct internal_state *state;

    uLong   adler;
} z_stream;


typedef z_stream *z_streamp;

#define MAX_MATCH  258
#define MIN_MATCH  3

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
#define Z_TREES         6

extern void deflate( z_streamp strm );

static inline uLong compressBound( uLong sourceLen )
{
    return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) + (sourceLen >> 25) + 13;
}
