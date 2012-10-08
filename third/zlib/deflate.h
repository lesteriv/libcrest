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
#define LENGTH_CODES	29
#define LITERALS		256
#define L_CODES			( LITERALS + 1 + LENGTH_CODES )
#define D_CODES			30
#define BL_CODES		19
#define HEAP_SIZE		( 2 * L_CODES + 1 )
#define MAX_BITS		15
#define ZBUF_SIZE		16

/**********************************************************************************************/
#define INIT_STATE		42
#define BUSY_STATE		113
#define FINISH_STATE	666


/**********************************************************************************************/
typedef struct ct_data_s
{
    union
	{
        unsigned short  freq;       
        unsigned short  code;       
    }
	fc;
    
	union
	{
        unsigned short  dad;        
        unsigned short  len;        
    } 
	dl;
}
ct_data;

/**********************************************************************************************/
#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

/**********************************************************************************************/
typedef struct static_tree_desc_s static_tree_desc;

/**********************************************************************************************/
typedef struct tree_desc_s
{
    ct_data*			dyn_tree;           
    int					max_code;            
    static_tree_desc*	stat_desc; 
}
tree_desc;

/**********************************************************************************************/
typedef unsigned		IPos;
typedef unsigned short	Pos;
typedef Pos				Posf;


/**********************************************************************************************/
typedef struct internal_state 
{
    long				block_start;
    Posf*				head; 
    unsigned int		ins_h;          
    unsigned int		lookahead;              
    unsigned int		match_length;           
    unsigned int		match_start;            
    unsigned int		pending;      
    byte*				pending_buf;  
    byte*				pending_out;  
    int					status;        
    z_stream*			strm;      
    unsigned int		strstart;               
    byte*				window;
    int					wrap;          

#define max_insert_length max_lazy_match
    
    ct_data_s			bl_tree[ 2 * BL_CODES + 1 ];  
    ct_data_s			dyn_dtree[ 2 * D_CODES + 1 ]; 
	ct_data_s			dyn_ltree[ HEAP_SIZE ];

    tree_desc_s			bl_desc;              
    tree_desc_s			d_desc;               
    tree_desc_s			l_desc;               

    unsigned short		bl_count[ MAX_BITS + 1 ];
    
    int					heap[ 2 * L_CODES + 1 ];      
    int					heap_len;
    int					heap_max;               
    
    unsigned short		bi_buf;
    int					bi_valid;
    unsigned short*		d_buf;
    unsigned char		depth[ 2 * L_CODES + 1 ];
    unsigned long		high_water;
	unsigned int		insert;        
    unsigned char*		l_buf;          
    unsigned int		last_lit;      
    unsigned int		lit_bufsize;
    unsigned long		opt_len;        
	unsigned long		static_len;     
}
deflate_state;


/**********************************************************************************************/
#define MAX_DIST(s)		( ( 1 << 15 ) - MIN_LOOKAHEAD )
#define WIN_INIT		258
#define MIN_LOOKAHEAD	( 258 + 3 + 1 )
#define put_byte(s, c)	{ s->pending_buf[ s->pending++ ] = ( c ); }


/**********************************************************************************************/
void _tr_init			( deflate_state* s );
void _tr_flush_block	( deflate_state* s, char *buf, size_t stored_len, int last );
void _tr_flush_bits		( deflate_state* s );
void _tr_stored_block	( deflate_state* s, char *buf, size_t stored_len, int last );


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
		s->d_buf[ s->last_lit ] = 0; \
		s->l_buf[ s->last_lit++ ] = cc; \
		s->dyn_ltree[ cc ].Freq++; \
		flush = ( s->last_lit == s->lit_bufsize - 1 ); \
	}

/**********************************************************************************************/
#define _tr_tally_dist( s, distance, length, flush ) \
	{ \
		unsigned char len = ( length ); \
		unsigned short dist = ( distance ); \
		s->d_buf[ s->last_lit ] = dist; \
		s->l_buf[ s->last_lit++ ] = len; \
		dist--; \
		s->dyn_ltree[ _length_code[ len ] + LITERALS + 1 ].Freq++; \
		s->dyn_dtree[ d_code(dist) ].Freq++; \
		flush = ( s->last_lit == s->lit_bufsize - 1 ); \
	}
