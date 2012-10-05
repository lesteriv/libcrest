#pragma once

#include "zlib.h"


#define LENGTH_CODES 29
#define LITERALS  256
#define L_CODES (LITERALS+1+LENGTH_CODES)
#define D_CODES   30
#define BL_CODES  19
#define HEAP_SIZE (2*L_CODES+1)
#define MAX_BITS 15
#define Buf_size 16


#define INIT_STATE    42
#define EXTRA_STATE   69
#define NAME_STATE    73
#define COMMENT_STATE 91
#define HCRC_STATE   103
#define BUSY_STATE   113
#define FINISH_STATE 666


typedef struct ct_data_s {
    union {
        unsigned short  freq;       
        unsigned short  code;       
    } fc;
    union {
        unsigned short  dad;        
        unsigned short  len;        
    } dl;
} ct_data;


#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

typedef struct static_tree_desc_s  static_tree_desc;

typedef struct tree_desc_s {
    ct_data *dyn_tree;           
    int     max_code;            
    static_tree_desc *stat_desc; 
}
tree_desc;


typedef unsigned short Pos;
typedef Pos Posf;
typedef unsigned IPos;


typedef struct internal_state {
    z_stream* strm;      
    int   status;        
    Byte *pending_buf;  
    unsigned long   pending_buf_size; 
    Byte *pending_out;  
    uInt   pending;      
    int   wrap;          

    uInt  w_size;        
    uInt  w_bits;        

    Byte *window;
    unsigned long window_size;

    Posf *prev;
    Posf *head; 

    uInt  ins_h;          
    uInt  hash_size;      
    uInt  hash_bits;      
    uInt  hash_mask;      
    uInt  hash_shift;
    

    long block_start;
    
    uInt match_length;           
    uInt strstart;               
    uInt match_start;            
    uInt lookahead;              
    
#   define max_insert_length  max_lazy_match
    
    struct ct_data_s dyn_ltree[HEAP_SIZE];   
    struct ct_data_s dyn_dtree[2*D_CODES+1]; 
    struct ct_data_s bl_tree[2*BL_CODES+1];  

    struct tree_desc_s l_desc;               
    struct tree_desc_s d_desc;               
    struct tree_desc_s bl_desc;              

    unsigned short bl_count[MAX_BITS+1];
    
    int heap[2*L_CODES+1];      
    int heap_len;               
    int heap_max;               
    
    unsigned char depth[2*L_CODES+1];
    unsigned char *l_buf;          
    uInt lit_bufsize;
    uInt last_lit;      
    unsigned short *d_buf;
    unsigned long opt_len;        
	unsigned long static_len;     
	uInt matches;       
	uInt insert;        
    unsigned short bi_buf;
    int bi_valid;
    unsigned long high_water;
} deflate_state;


#define put_byte(s, c) {s->pending_buf[s->pending++] = (c);}
#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
#define MAX_DIST(s)  ((s)->w_size-MIN_LOOKAHEAD)
#define WIN_INIT MAX_MATCH

        
void _tr_init (deflate_state *s);
int _tr_tally (deflate_state *s, unsigned dist, unsigned lc);
void _tr_flush_block (deflate_state *s, char *buf,
                        unsigned long stored_len, int last);
void _tr_flush_bits (deflate_state *s);
void _tr_align (deflate_state *s);
void _tr_stored_block (deflate_state *s, char *buf,
                        unsigned long stored_len, int last);

#define d_code(dist) \
   ((dist) < 256 ? _dist_code[dist] : _dist_code[256+((dist)>>7)])

extern const unsigned char _length_code[];
extern const unsigned char _dist_code[];

# define _tr_tally_lit(s, c, flush) \
  { unsigned char cc = (c); \
    s->d_buf[s->last_lit] = 0; \
    s->l_buf[s->last_lit++] = cc; \
    s->dyn_ltree[cc].Freq++; \
    flush = (s->last_lit == s->lit_bufsize-1); \
   }

# define _tr_tally_dist(s, distance, length, flush) \
  { unsigned char len = (length); \
    unsigned short dist = (distance); \
    s->d_buf[s->last_lit] = dist; \
    s->l_buf[s->last_lit++] = len; \
    dist--; \
    s->dyn_ltree[_length_code[len]+LITERALS+1].Freq++; \
    s->dyn_dtree[d_code(dist)].Freq++; \
    flush = (s->last_lit == s->lit_bufsize-1); \
  }
