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
#include <cstddef>
#include <cstdint>

/**********************************************************************************************/
typedef unsigned char byte;

/**********************************************************************************************/
struct internal_state;


/**********************************************************************************************/
#define LENGTH_CODES	29
#define LITERALS		256
#define ZBUF_SIZE		16
#define D_CODES			30
#define BL_CODES		19
#define HEAP_SIZE		( 2 * L_CODES + 1 )
#define L_CODES			( LITERALS + 1 + LENGTH_CODES )
#define MAX_BITS		15


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
struct z_stream
{
    uint32_t			adler;
    size_t				avail_in;
    size_t				avail_out;
    const byte*			next_in;
    byte*				next_out;
	
    long				block_start;
    unsigned short*		head; 
    unsigned int		ins_h;          
    unsigned int		lookahead;              
    unsigned int		match_length;           
    unsigned int		match_start;            
    unsigned int		pending;      
    byte*				pending_buf;  
    byte*				pending_out;  
    int					status;        
    unsigned int		strstart;               
    byte*				window;
    int					wrap;          
    
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
};


/**********************************************************************************************/
extern void deflate( z_stream& strm );
