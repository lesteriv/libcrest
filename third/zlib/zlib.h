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

typedef struct gz_header_s {
    int     text;
    uLong   time;
    int     xflags;
    int     os;
    Bytef   *extra;
    uInt    extra_len;
    uInt    extra_max;
    Bytef   *name;
    uInt    name_max;
    Bytef   *comment;
    uInt    comm_max;
    int     hcrc;
    int     done;
} gz_header;

typedef gz_header FAR *gz_headerp;



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

#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_RLE                 3
#define Z_FIXED               4
#define Z_DEFAULT_STRATEGY    0

#define Z_BINARY   0
#define Z_TEXT     1
#define Z_ASCII    Z_TEXT
#define Z_UNKNOWN  2

#define Z_DEFLATED   8

#define Z_NULL  0

#define zlib_version zlibVersion()



ZEXTERN int deflate OF((z_streamp strm, int flush));

ZEXTERN int deflateEnd OF((z_streamp strm));

ZEXTERN int inflate OF((z_streamp strm, int flush));

ZEXTERN int inflateEnd OF((z_streamp strm));

ZEXTERN int deflateCopy OF((z_streamp dest,
                                    z_streamp source));

ZEXTERN int deflateReset OF((z_streamp strm));

ZEXTERN int deflateParams OF((z_streamp strm,
                                      int level,
                                      int strategy));

ZEXTERN int deflateTune OF((z_streamp strm,
                                    int good_length,
                                    int max_lazy,
                                    int nice_length,
                                    int max_chain));

ZEXTERN uLong deflateBound OF((z_streamp strm,
                                       uLong sourceLen));

ZEXTERN int deflatePending OF((z_streamp strm,
                                       unsigned *pending,
                                       int *bits));

ZEXTERN int deflatePrime OF((z_streamp strm,
                                     int bits,
                                     int value));

ZEXTERN int deflateSetHeader OF((z_streamp strm,
                                         gz_headerp head));

ZEXTERN int inflateSetDictionary OF((z_streamp strm,
                                             const Bytef *dictionary,
                                             uInt  dictLength));

ZEXTERN int inflateSync OF((z_streamp strm));

ZEXTERN int inflateCopy OF((z_streamp dest,
                                    z_streamp source));

ZEXTERN int inflateReset OF((z_streamp strm));

ZEXTERN int inflateReset2 OF((z_streamp strm,
                                      int windowBits));

ZEXTERN int inflatePrime OF((z_streamp strm,
                                     int bits,
                                     int value));

ZEXTERN long inflateMark OF((z_streamp strm));

ZEXTERN int inflateGetHeader OF((z_streamp strm,
                                         gz_headerp head));

typedef unsigned (*in_func) OF((void FAR *, unsigned char FAR * FAR *));
typedef int (*out_func) OF((void FAR *, unsigned char FAR *, unsigned));

ZEXTERN int inflateBack OF((z_streamp strm,
                                    in_func in, void FAR *in_desc,
                                    out_func out, void FAR *out_desc));

ZEXTERN int inflateBackEnd OF((z_streamp strm));

ZEXTERN uLong adler32 OF((uLong adler, const Bytef *buf, uInt len));


ZEXTERN int deflateInit_ OF((z_streamp strm, int level,
                                     const char *version, int stream_size));
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
