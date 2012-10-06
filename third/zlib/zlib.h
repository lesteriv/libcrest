/**********************************************************************************************/
/* zlib.h			  		                                                   				  */
/*                                                                       					  */
/* (C) 1995-2012 Jean-loup Gailly and Mark Adler											  */
/* ZLIB license   																		  	  */
/**********************************************************************************************/

/*
  Copyright (C) 1995-2012 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu


  The data format used by the zlib library is described by RFCs (Request for
  Comments) 1950 to 1952 in the files http://tools.ietf.org/html/rfc1950
  (zlib format), rfc1951 (deflate format) and rfc1952 (gzip format).
*/

#pragma once

// STD
#include <stddef.h>

/**********************************************************************************************/
typedef unsigned char Byte;

/**********************************************************************************************/
struct internal_state;

/**********************************************************************************************/
struct z_stream
{
    unsigned long   adler;
    unsigned int    avail_in;
    unsigned int	avail_out;
    const Byte*		next_in;
    Byte*			next_out;
    internal_state*	state;
};

/**********************************************************************************************/
#define MAX_MATCH	258
#define MIN_MATCH	3


/**********************************************************************************************/
extern void deflate( z_stream* strm );

/**********************************************************************************************/
static inline size_t compressBound( size_t sourceLen )
{
    return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) + (sourceLen >> 25) + 13;
}
