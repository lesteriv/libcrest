/**********************************************************************************************/
/* zlib.h			  		                                                   				  */
/*                                                                       					  */
/* (C) 1995-2012 Jean-loup Gailly and Mark Adler											  */
/* ZLIB license   																		  	  */
/**********************************************************************************************/

#pragma once

// STD
#include <stddef.h>
#include <stdint.h>

struct internal_state;

typedef unsigned char  Byte;
typedef unsigned int   uInt;

typedef uInt		uIntf;

typedef uint32_t z_crc_t;


typedef struct z_stream_s {
    const Byte *next_in;
    uInt     avail_in;

    Byte    *next_out;
    uInt     avail_out;

    internal_state *state;

    unsigned long   adler;
} z_stream;


#define MAX_MATCH  258
#define MIN_MATCH  3

#define Z_NO_FLUSH      0
#define Z_FINISH        4


extern void deflate( z_stream* strm );

static inline size_t compressBound( size_t sourceLen )
{
    return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) + (sourceLen >> 25) + 13;
}
