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
    z_const Bytef *next_in;
    uInt     avail_in;
    uLong    total_in;

    Bytef    *next_out;
    uInt     avail_out;
    uLong    total_out;

    struct internal_state FAR *state;

    int     data_type;
    uLong   adler;
    uLong   reserved;
} z_stream;


typedef z_stream FAR *z_streamp;


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

#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)

#define Z_BINARY   0
#define Z_TEXT     1
#define Z_ASCII    Z_TEXT
#define Z_UNKNOWN  2

#define Z_DEFLATED   8
#define Z_NULL  0


ZEXTERN int deflate OF((z_streamp strm, int flush));
ZEXTERN int deflateEnd OF((z_streamp strm));
ZEXTERN int deflateInit_ OF((z_streamp strm, int level,
                                     const char *version, int stream_size));

ZEXTERN uLong adler32 OF((uLong adler, const Bytef *buf, uInt len));

#define deflateInit(strm, level) \
        deflateInit_((strm), (level), ZLIB_VERSION, (int)sizeof(z_stream))

static inline uLong compressBound (uLong sourceLen)
{
    return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) +
           (sourceLen >> 25) + 13;
}
	
#ifdef __cplusplus
}
#endif
