#pragma once

#include "zconf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZLIB_VERSION "1.2.7"
#define ZLIB_VERNUM 0x1270
#define ZLIB_VER_MAJOR 1
#define ZLIB_VER_MINOR 2
#define ZLIB_VER_REVISION 7
#define ZLIB_VER_SUBREVISION 0

struct internal_state;

typedef struct z_stream_s {
    const Bytef *next_in;
    uInt     avail_in;

    Bytef    *next_out;
    uInt     avail_out;

    struct internal_state *state;

    uLong   adler;
} z_stream;


typedef z_stream *z_streamp;


#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
#define Z_TREES         6

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

#define Z_DEFLATED   8
#define Z_NULL  0


extern int deflate (z_streamp strm);
extern int deflateEnd (z_streamp strm);
extern int deflateInit_ (z_streamp strm,
                                     const char *version, int stream_size);

extern uLong adler32 (uLong adler, const Bytef *buf, uInt len);

#define deflateInit(strm) \
        deflateInit_((strm), ZLIB_VERSION, (int)sizeof(z_stream))

static inline uLong compressBound (uLong sourceLen)
{
    return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) +
           (sourceLen >> 25) + 13;
}
	
#ifdef __cplusplus
}
#endif
