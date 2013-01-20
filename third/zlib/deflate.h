/**********************************************************************************************/
/* deflate.h		  		                                                   				  */
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

// ZLIB
#include "zlib.h"


/**********************************************************************************************/
#define INIT_STATE		42
#define BUSY_STATE		113
#define FINISH_STATE	666


/**********************************************************************************************/
#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len


/**********************************************************************************************/
typedef unsigned		IPos;
typedef unsigned short	Pos;
typedef Pos				Posf;


/**********************************************************************************************/
#define MAX_DIST(s)		( ( 1 << 15 ) - MIN_LOOKAHEAD )
#define WIN_INIT		258
#define MIN_LOOKAHEAD	( 258 + 3 + 1 )
#define put_byte(s, c)	{ s.pending_buf[ s.pending++ ] = ( c ); }


/**********************************************************************************************/
void _tr_init			( z_stream& s );
void _tr_flush_block	( z_stream& s, char *buf, size_t stored_len, int last );
void _tr_flush_bits		( z_stream& s );
void _tr_stored_block	( z_stream& s, char *buf, size_t stored_len, int last );


/**********************************************************************************************/
#define d_code(dist) \
   ( ( dist ) < 256 ? _dist_code[ dist ] : _dist_code[ 256 + ( (dist) >> 7 ) ] )

/**********************************************************************************************/
extern const unsigned char _dist_code[];
extern const unsigned char _length_code[];

/**********************************************************************************************/
#define _tr_tally_lit( s, c, flush ) \
	{ \
		unsigned char cc = ( c ); \
		s.d_buf[ s.last_lit ] = 0; \
		s.l_buf[ s.last_lit++ ] = cc; \
		s.dyn_ltree[ cc ].Freq++; \
		flush = ( s.last_lit == s.lit_bufsize - 1 ); \
	}

/**********************************************************************************************/
#define _tr_tally_dist( s, distance, length, flush ) \
	{ \
		unsigned char len = ( length ); \
		unsigned short dist = ( distance ); \
		s.d_buf[ s.last_lit ] = dist; \
		s.l_buf[ s.last_lit++ ] = len; \
		dist--; \
		s.dyn_ltree[ _length_code[ len ] + LITERALS + 1 ].Freq++; \
		s.dyn_dtree[ d_code(dist) ].Freq++; \
		flush = ( s.last_lit == s.lit_bufsize - 1 ); \
	}
