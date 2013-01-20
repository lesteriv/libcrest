/**********************************************************************************************/
/* deflate.cpp		  		                                                   				  */
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

// STD
#include <stdlib.h>
#include <string.h>

// ZLIB
#include "deflate.h"


/**********************************************************************************************/
#define BASE 65521      /* largest prime smaller than 65536 */
#define NMAX 5552		/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

/**********************************************************************************************/
#define Z_DEFLATED 8

/**********************************************************************************************/
#define DO1( buf, i )	{ adler += (buf)[ i ]; sum2 += adler; }
#define DO2( buf, i )	DO1( buf, i ); DO1( buf, i + 1 );
#define DO4( buf, i )	DO2( buf, i ); DO2( buf, i + 2 );
#define DO8( buf, i )	DO4( buf, i ); DO4( buf, i + 4 );
#define DO16( buf )		DO8( buf, 0 ); DO8( buf, 8 );

/**********************************************************************************************/
#define MOD( a )		a %= BASE
#define MOD28( a )		a %= BASE


/**********************************************************************************************/
static uint32_t adler32(
    uint32_t	adler,
    const byte*	buf,
    size_t		len )
{
    /* split Adler-32 into component sums */
    uint32_t sum2 = ( adler >> 16 ) & 0xffff;
    adler &= 0xffff;

    /* in case user likes doing a byte at a time, keep it fast */
    if( len == 1 )
	{
        adler += buf[ 0 ];
        if( adler >= BASE )
            adler -= BASE;
		
        sum2 += adler;
        if( sum2 >= BASE )
            sum2 -= BASE;
		
        return adler | ( sum2 << 16 );
    }

    /* initial Adler-32 value (deferred check for len == 1 speed) */
    if( !buf )
        return 1L;

    /* in case short lengths are provided, keep it somewhat fast */
    if( len < 16 )
	{
        while( len-- )
		{
            adler += *buf++;
            sum2 += adler;
        }
        
		if( adler >= BASE )
            adler -= BASE;
		
        MOD28( sum2 );          /* only added so many BASE's */
        return adler | ( sum2 << 16 );
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while( len >= NMAX )
	{
        len -= NMAX;
        size_t n = NMAX / 16;   /* NMAX is divisible by 16 */
        do
		{
            DO16( buf );        /* 16 sums unrolled */
            buf += 16;
        }
		while( --n );
		
        MOD( adler );
        MOD( sum2 );
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if( len ) 
	{   /* avoid modulos if none remaining */
        while( len >= 16 )
		{
            len -= 16;
            DO16( buf);
            buf += 16;
        }
		
        while( len-- )
		{
            adler += *buf++;
            sum2 += adler;
        }
		
        MOD( adler );
        MOD( sum2 );
    }

    /* return recombined sums */
    return adler | ( sum2 << 16 );
}

/**********************************************************************************************/
typedef enum
{
    need_more,      
    block_done,     
    finish_started, 
    finish_done     
}
block_state;

/**********************************************************************************************/
static block_state deflate_fast( z_stream& s, int flush );


/**********************************************************************************************/
#define CLEAR_HASH( s ) \
    s.head[ ( 1 << 15 ) - 1 ] = 0; \
    memset( s.head, 0, (unsigned) ( ( 1 << 15 ) - 1 ) * sizeof( *s.head ) );

/**********************************************************************************************/
#define INSERT_STRING( s, str, match_head ) \
   (UPDATE_HASH( s, s.ins_h, s.window[ (str) + 2 ] ), \
    match_head = s.head[s.ins_h ], \
    s.head[ s.ins_h ] = (Pos)( str ))

/**********************************************************************************************/
#define UPDATE_HASH( s, h, c ) ( h = ( ((h)<<( ( 15 + 2 ) / 3 )) ^ (c) ) & ( ( 1 << 15 ) - 1 ) )


/**********************************************************************************************/
static void deflateResetKeep( z_stream& s )
{
    s.pending = 0;
    s.pending_out = s.pending_buf;

    if( s.wrap < 0 )
        s.wrap = -s.wrap; 

    s.status = s.wrap ? INIT_STATE : BUSY_STATE;
    s.adler = 1;

    _tr_init( s );
}

/**********************************************************************************************/
static void lm_init( z_stream& s )
{
    CLEAR_HASH( s );

    s.strstart		= 0;
    s.block_start	= 0L;
    s.lookahead		= 0;
    s.insert		= 0;
    s.match_length	= 2;
    s.ins_h			= 0;
}

/**********************************************************************************************/
static void deflateReset( z_stream& s )
{
    deflateResetKeep( s );
    lm_init( s );
}
	
/**********************************************************************************************/
void deflateInit( z_stream& s )
{
    s.wrap	  = 1;
    s.window = (byte*) malloc( ( 1 << 15 ) * 2 * sizeof( byte ) );
    s.head   = (Posf*) malloc( ( 1 << 15 ) * sizeof( Pos ) );

    s.high_water = 0;      
    s.lit_bufsize = 1 << (8 + 6); 

    unsigned short* overlay = (unsigned short*) malloc( s.lit_bufsize * ( sizeof(unsigned short) + 2 ) );
    s.pending_buf = (unsigned char*) overlay;
    s.d_buf = overlay + s.lit_bufsize / sizeof(unsigned short);
    s.l_buf = s.pending_buf + ( 1 + sizeof(unsigned short) ) * s.lit_bufsize;

    deflateReset( s );
}

/**********************************************************************************************/
static void putShortMSB(
    z_stream&		s,
    unsigned int	b )
{
    put_byte( s, (byte) ( b >> 8 ) );
    put_byte( s, (byte) ( b & 0xff ) );
}

/**********************************************************************************************/
static void flush_pending( z_stream& s )
{
    unsigned len;

    _tr_flush_bits( s );
    len = s.pending;
    if( len > s.avail_out ) len = s.avail_out;
    if( !len )
		return;

    memmove( s.next_out, s.pending_out, len );
    s.next_out		+= len;
    s.pending_out	+= len;
    s.avail_out		-= len;
    s.pending		-= len;
	
    if( !s.pending )
        s.pending_out = s.pending_buf;
}

/**********************************************************************************************/
void deflate( z_stream& s )
{
	deflateInit( s );
	
    if( s.status == INIT_STATE )
	{
		unsigned int header = ( Z_DEFLATED + ( 7 << 4 ) ) << 8;
		if( s.strstart ) header |= 0x20;
		header += 31 - ( header % 31 );

		s.status = BUSY_STATE;
		putShortMSB( s, header );

		if( s.strstart )
		{
			putShortMSB( s, (unsigned int) ( s.adler >> 16 ) );
			putShortMSB( s, (unsigned int) ( s.adler & 0xffff ) );
		}
		
		s.adler = 1;
	}
    
    if( s.pending )
	{
        flush_pending( s );
        if( !s.avail_out )
            goto finish;
    }
    
    if( s.avail_in || s.lookahead || s.status != FINISH_STATE )
	{
        block_state bstate = deflate_fast( s, 4 );

        if( bstate == finish_started || bstate == finish_done )
            s.status = FINISH_STATE;

        if( bstate == need_more || bstate == finish_started )
            goto finish;

        if( bstate == block_done )
		{
			_tr_stored_block( s, (char*) 0, 0L, 0 );
            flush_pending( s );
			
            if( !s.avail_out )
				goto finish;
        }
    }

	putShortMSB( s, (unsigned int) ( s.adler >> 16 ) );
	putShortMSB( s, (unsigned int) ( s.adler & 0xffff ) );
    flush_pending( s );
    
finish:
    free( s.pending_buf );
    free( s.head );
    free( s.window );
}

/**********************************************************************************************/
static int read_buf(
    z_stream&	s,
    byte*		buf,
    unsigned	size )
{
    unsigned len = s.avail_in;

    if( len > size ) len = size;
    if( !len ) return 0;

    s.avail_in -= len;

    memmove( buf, s.next_in, len );
    if( s.wrap == 1 )
        s.adler = adler32( s.adler, buf, len );

    s.next_in += len;
    return (int) len;
}

/**********************************************************************************************/
static unsigned int longest_match( z_stream& s, IPos cur_match )                           
{
    register byte* scan = s.window + s.strstart; 
    register byte* match;                       
    register int len;                           
    register byte* strend = s.window + s.strstart + 258;

    match = s.window + cur_match;

    if( match[ 0 ] != scan[ 0 ] || match[ 1 ] != scan[ 1 ] )
		return 2;

    scan += 2, match += 2;
    
    do {}
    while(
		*++scan == *++match && *++scan == *++match &&
        *++scan == *++match && *++scan == *++match &&
        *++scan == *++match && *++scan == *++match &&
        *++scan == *++match && *++scan == *++match &&
        scan < strend );

    len = 258 - (int)( strend - scan );

    if( len < 3 )
		return 2;

    s.match_start = cur_match;
    return (unsigned int) len <= s.lookahead ? (unsigned int) len : s.lookahead;
}

/**********************************************************************************************/
static void fill_window( z_stream& s )
{
    register unsigned n, m;
    register Posf* p;
    unsigned more;    
    unsigned int wsize = 1 << 15;

    do
	{
        more = (unsigned) ( ( (unsigned long) 2L * ( 1 << 15 ) ) - (unsigned long) s.lookahead - (unsigned long) s.strstart );
        
        if( s.strstart >= wsize + MAX_DIST( s ) )
		{
            memmove( s.window, s.window+wsize, (unsigned) wsize );
            s.match_start -= wsize;
            s.strstart    -= wsize; 
            s.block_start -= (long) wsize;

            n = 1 << 15;
            p = &s.head[ n ];
			
            do
			{
                m = *--p;
                *p = (Pos) ( m >= wsize ? m-wsize : 0 );
            }
			while( --n );

            n = wsize;
            more += wsize;
        }
		
        if( !s.avail_in )
			break;

        n = read_buf( s, s.window + s.strstart + s.lookahead, more );
        s.lookahead += n;
        
        if( s.lookahead + s.insert >= 3 )
		{
            unsigned int str = s.strstart - s.insert;
            s.ins_h = s.window[ str ];
            UPDATE_HASH( s, s.ins_h, s.window[ str + 1 ] );
            
			while( s.insert )
			{
                UPDATE_HASH( s, s.ins_h, s.window[ str + 2 ] );
                s.head[ s.ins_h ] = (Pos) str;
                str++;
                s.insert--;
				
                if( s.lookahead + s.insert < 3 )
                    break;
            }
        }
    }
	while( s.lookahead < MIN_LOOKAHEAD && s.avail_in );
    
    if( s.high_water < ( (unsigned long) 2L * ( 1 << 15 ) ) )
	{
        unsigned long curr = s.strstart + (unsigned long) ( s.lookahead );
        unsigned long init;

        if( s.high_water < curr )
		{
            init = ( (unsigned long) 2L * ( 1 << 15 ) ) - curr;
            if( init > WIN_INIT )
                init = WIN_INIT;
			
            memset( s.window + curr, 0, (unsigned) init );
            s.high_water = curr + init;
        }
        else if( s.high_water < (unsigned long) curr + WIN_INIT )
		{
            init = (unsigned long) curr + WIN_INIT - s.high_water;
            if( init > ( (unsigned long) 2L * ( 1 << 15 ) ) - s.high_water )
                init = ( (unsigned long) 2L * ( 1 << 15 ) ) - s.high_water;
			
            memset( s.window + s.high_water, 0, (unsigned) init );
            s.high_water += init;
        }
    }
}

/**********************************************************************************************/
#define FLUSH_BLOCK_ONLY( s, last ) \
{ \
   _tr_flush_block( \
		s, \
		( s.block_start >= 0L ? \
			(char*) &s.window[ (unsigned)s.block_start ] : \
            (char*) 0 ), \
		(unsigned long) ( (long) s.strstart - s.block_start ), \
		(last) ); \
	s.block_start = s.strstart; \
	flush_pending( s ); \
}

/**********************************************************************************************/
#define FLUSH_BLOCK( s, last ) \
{ \
   FLUSH_BLOCK_ONLY(s, last); \
   if( !s.avail_out ) \
   return (last) ? finish_started : need_more; \
}

/**********************************************************************************************/
static block_state deflate_fast( z_stream& s, int flush )
{
    IPos hash_head;
    int bflush;       

    for(;;)
	{
        if( s.lookahead < MIN_LOOKAHEAD )
		{
            fill_window( s );
            if( s.lookahead < MIN_LOOKAHEAD && flush == 0 )
                return need_more;

			if( s.lookahead == 0 )
				break; 
        }
        
        hash_head = 0;
        if( s.lookahead >= 3 )
            INSERT_STRING( s, s.strstart, hash_head );
        
		if( hash_head != 0 && s.strstart - hash_head <= MAX_DIST( s ) )
            s.match_length = longest_match( s, hash_head );
		
		if( s.match_length >= 3 )
		{
            _tr_tally_dist( s, s.strstart - s.match_start, s.match_length - 3, bflush );

            s.lookahead -= s.match_length;
			s.strstart += s.match_length;
			s.match_length = 0;
			s.ins_h = s.window[ s.strstart ];
			UPDATE_HASH( s, s.ins_h, s.window[ s.strstart + 1 ] );
        }
		else
		{
            _tr_tally_lit( s, s.window[ s.strstart ], bflush );
            s.lookahead--;
            s.strstart++;
        }
		
        if( bflush )
			FLUSH_BLOCK( s, 0 );
    }
	
    s.insert = s.strstart < 2 ? s.strstart : 2;
    if( flush == 4 )
	{
        FLUSH_BLOCK( s, 1 );
        return finish_done;
    }
	
    if( s.last_lit )
        FLUSH_BLOCK( s, 0 );
	
    return block_done;
}
