/**********************************************************************************************/
/* trees.cpp		  		                                                   				  */
/*                                                                       					  */
/* (C) 1995-2012 Jean-loup Gailly and Mark Adler											  */
/* ZLIB license   																		  	  */
/**********************************************************************************************/

// ZLIB
#include "deflate.h"
#include "trees.h"


/**********************************************************************************************/
struct static_tree_desc_s
{
    int				elems;               
    const int*		extra_bits;      
    int				extra_base;          
    int				max_length;          
    const ct_data*	static_tree;  
};

/**********************************************************************************************/
static static_tree_desc  static_l_desc = {
	L_CODES, extra_lbits, LITERALS + 1, MAX_BITS, static_ltree };

/**********************************************************************************************/
static static_tree_desc  static_d_desc = {
	D_CODES, extra_dbits, 0, MAX_BITS, static_dtree };

/**********************************************************************************************/
static static_tree_desc  static_bl_desc = {
	BL_CODES, extra_blbits, 0, MAX_BL_BITS, (const ct_data*) 0 };


static void pqdownheap     (deflate_state *s, ct_data *tree, int k);
static void gen_bitlen     (deflate_state *s, tree_desc *desc);
static void gen_codes      (ct_data *tree, int max_code, unsigned short *bl_count);
static void build_tree     (deflate_state *s, tree_desc *desc);
static void scan_tree      (deflate_state *s, ct_data *tree, int max_code);
static void send_tree      (deflate_state *s, ct_data *tree, int max_code);
static int  build_bl_tree  (deflate_state *s);
static void send_all_trees (deflate_state *s, int lcodes, int dcodes,
                              int blcodes);
static void compress_block (deflate_state *s, ct_data *ltree,
                              ct_data *dtree);
static unsigned bi_reverse (unsigned value, int length);
static void bi_windup      (deflate_state *s);
static void bi_flush       (deflate_state *s);
static void copy_block     (deflate_state *s, char *buf, unsigned len,
                              int header);

#  define send_code(s, c, tree) send_bits(s, tree[c].Code, tree[c].Len)

#define put_short(s, w) { \
    put_byte(s, (unsigned char)((w) & 0xff)); \
    put_byte(s, (unsigned char)((unsigned short)(w) >> 8)); \
}

#define send_bits(s, value, length) \
{ int len = length;\
  if (s->bi_valid > (int)Buf_size - len) {\
    int val = value;\
    s->bi_buf |= (unsigned short)val << s->bi_valid;\
    put_short(s, s->bi_buf);\
    s->bi_buf = (unsigned short)val >> (Buf_size - s->bi_valid);\
    s->bi_valid += len - Buf_size;\
  } else {\
    s->bi_buf |= (unsigned short)(value) << s->bi_valid;\
    s->bi_valid += len;\
  }\
}

static void init_block( deflate_state* s)
{
    int n; 
    
    for (n = 0; n < L_CODES;  n++) s->dyn_ltree[n].Freq = 0;
    for (n = 0; n < D_CODES;  n++) s->dyn_dtree[n].Freq = 0;
    for (n = 0; n < BL_CODES; n++) s->bl_tree[n].Freq = 0;

    s->dyn_ltree[END_BLOCK].Freq = 1;
    s->opt_len = s->static_len = 0L;
    s->last_lit = s->matches = 0;
}

void _tr_init( deflate_state* s )
{
    s->l_desc.dyn_tree = s->dyn_ltree;
    s->l_desc.stat_desc = &static_l_desc;

    s->d_desc.dyn_tree = s->dyn_dtree;
    s->d_desc.stat_desc = &static_d_desc;

    s->bl_desc.dyn_tree = s->bl_tree;
    s->bl_desc.stat_desc = &static_bl_desc;

    s->bi_buf = 0;
    s->bi_valid = 0;
    
    init_block(s);
}


#define SMALLEST 1




#define pqremove(s, tree, top) \
{\
    top = s->heap[SMALLEST]; \
    s->heap[SMALLEST] = s->heap[s->heap_len--]; \
    pqdownheap(s, tree, SMALLEST); \
}


#define smaller(tree, n, m, depth) \
   (tree[n].Freq < tree[m].Freq || \
   (tree[n].Freq == tree[m].Freq && depth[n] <= depth[m]))


static void pqdownheap(
    deflate_state *s,
    ct_data *tree,
    int k )       
{
    int v = s->heap[k];
    int j = k << 1;  
    while (j <= s->heap_len) {
        
        if (j < s->heap_len &&
            smaller(tree, s->heap[j+1], s->heap[j], s->depth)) {
            j++;
        }
        
        if (smaller(tree, v, s->heap[j], s->depth)) break;

        
        s->heap[k] = s->heap[j];  k = j;

        
        j <<= 1;
    }
    s->heap[k] = v;
}


static void gen_bitlen(
    deflate_state *s,
    tree_desc *desc )   
{
    ct_data *tree        = desc->dyn_tree;
    int max_code         = desc->max_code;
    const ct_data *stree = desc->stat_desc->static_tree;
    const int *extra    = desc->stat_desc->extra_bits;
    int base             = desc->stat_desc->extra_base;
    int max_length       = desc->stat_desc->max_length;
    int h;              
    int n, m;           
    int bits;           
    int xbits;          
    unsigned short f;              
    int overflow = 0;   

    for (bits = 0; bits <= MAX_BITS; bits++) s->bl_count[bits] = 0;

    
    tree[s->heap[s->heap_max]].Len = 0; 

    for (h = s->heap_max+1; h < HEAP_SIZE; h++) {
        n = s->heap[h];
        bits = tree[tree[n].Dad].Len + 1;
        if (bits > max_length) bits = max_length, overflow++;
        tree[n].Len = (unsigned short)bits;
        

        if (n > max_code) continue; 

        s->bl_count[bits]++;
        xbits = 0;
        if (n >= base) xbits = extra[n-base];
        f = tree[n].Freq;
        s->opt_len += (unsigned long)f * (bits + xbits);
        if (stree) s->static_len += (unsigned long)f * (stree[n].Len + xbits);
    }
    if (overflow == 0) return;

    do {
        bits = max_length-1;
        while (s->bl_count[bits] == 0) bits--;
        s->bl_count[bits]--;      
        s->bl_count[bits+1] += 2; 
        s->bl_count[max_length]--;
        
        overflow -= 2;
    } while (overflow > 0);

    
    for (bits = max_length; bits != 0; bits--) {
        n = s->bl_count[bits];
        while (n != 0) {
            m = s->heap[--h];
            if (m > max_code) continue;
            if ((unsigned) tree[m].Len != (unsigned) bits) {
                s->opt_len += ((long)bits - (long)tree[m].Len)
                              *(long)tree[m].Freq;
                tree[m].Len = (unsigned short)bits;
            }
            n--;
        }
    }
}


static void gen_codes(
    ct_data *tree,          
    int max_code,             
    unsigned short *bl_count )
{
    unsigned short next_code[MAX_BITS+1]; 
    unsigned short code = 0;              
    int bits;                  
    int n;                     

    
    for (bits = 1; bits <= MAX_BITS; bits++) {
        next_code[bits] = code = (code + bl_count[bits-1]) << 1;
    }
    
    for (n = 0;  n <= max_code; n++) {
        int len = tree[n].Len;
        if (len == 0) continue;
        
        tree[n].Code = bi_reverse(next_code[len]++, len);
    }
}


static void build_tree(
    deflate_state *s,
    tree_desc *desc )
{
    ct_data *tree         = desc->dyn_tree;
    const ct_data *stree  = desc->stat_desc->static_tree;
    int elems             = desc->stat_desc->elems;
    int n, m;          
    int max_code = -1; 
    int node;          

    
    s->heap_len = 0, s->heap_max = HEAP_SIZE;

    for (n = 0; n < elems; n++) {
        if (tree[n].Freq != 0) {
            s->heap[++(s->heap_len)] = max_code = n;
            s->depth[n] = 0;
        } else {
            tree[n].Len = 0;
        }
    }

    
    while (s->heap_len < 2) {
        node = s->heap[++(s->heap_len)] = (max_code < 2 ? ++max_code : 0);
        tree[node].Freq = 1;
        s->depth[node] = 0;
        s->opt_len--; if (stree) s->static_len -= stree[node].Len;
        
    }
    desc->max_code = max_code;

    
    for (n = s->heap_len/2; n >= 1; n--) pqdownheap(s, tree, n);

    
    node = elems;              
    do {
        pqremove(s, tree, n);  
        m = s->heap[SMALLEST]; 

        s->heap[--(s->heap_max)] = n; 
        s->heap[--(s->heap_max)] = m;

        
        tree[node].Freq = tree[n].Freq + tree[m].Freq;
        s->depth[node] = (unsigned char)((s->depth[n] >= s->depth[m] ?
                                s->depth[n] : s->depth[m]) + 1);
        tree[n].Dad = tree[m].Dad = (unsigned short)node;
        
        s->heap[SMALLEST] = node++;
        pqdownheap(s, tree, SMALLEST);

    } while (s->heap_len >= 2);

    s->heap[--(s->heap_max)] = s->heap[SMALLEST];

    
    gen_bitlen(s, (tree_desc *)desc);

    
    gen_codes ((ct_data *)tree, max_code, s->bl_count);
}


static void scan_tree (
    deflate_state *s,
    ct_data *tree,
    int max_code )   
{
    int n;                     
    int prevlen = -1;          
    int curlen;                
    int nextlen = tree[0].Len; 
    int count = 0;             
    int max_count = 7;         
    int min_count = 4;         

    if (nextlen == 0) max_count = 138, min_count = 3;
    tree[max_code+1].Len = (unsigned short)0xffff; 

    for (n = 0; n <= max_code; n++) {
        curlen = nextlen; nextlen = tree[n+1].Len;
        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            s->bl_tree[curlen].Freq += count;
        } else if (curlen != 0) {
            if (curlen != prevlen) s->bl_tree[curlen].Freq++;
            s->bl_tree[REP_3_6].Freq++;
        } else if (count <= 10) {
            s->bl_tree[REPZ_3_10].Freq++;
        } else {
            s->bl_tree[REPZ_11_138].Freq++;
        }
        count = 0; prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    }
}


static void send_tree (
    deflate_state *s,
    ct_data *tree,
    int max_code )      
{
    int n;                     
    int prevlen = -1;          
    int curlen;                
    int nextlen = tree[0].Len; 
    int count = 0;             
    int max_count = 7;         
    int min_count = 4;         

      
    if (nextlen == 0) max_count = 138, min_count = 3;

    for (n = 0; n <= max_code; n++) {
        curlen = nextlen; nextlen = tree[n+1].Len;
        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            do { send_code(s, curlen, s->bl_tree); } while (--count != 0);

        } else if (curlen != 0) {
            if (curlen != prevlen) {
                send_code(s, curlen, s->bl_tree); count--;
            }
            send_code(s, REP_3_6, s->bl_tree); send_bits(s, count-3, 2);

        } else if (count <= 10) {
            send_code(s, REPZ_3_10, s->bl_tree); send_bits(s, count-3, 3);

        } else {
            send_code(s, REPZ_11_138, s->bl_tree); send_bits(s, count-11, 7);
        }
        count = 0; prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    }
}


static int build_bl_tree(
    deflate_state *s )
{
    int max_blindex;  
    
    scan_tree(s, (ct_data *)s->dyn_ltree, s->l_desc.max_code);
    scan_tree(s, (ct_data *)s->dyn_dtree, s->d_desc.max_code);
    build_tree(s, (tree_desc *)(&(s->bl_desc)));
    
    for (max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--) {
        if (s->bl_tree[bl_order[max_blindex]].Len != 0) break;
    }
    
    s->opt_len += 3*(max_blindex+1) + 5+5+4;
    return max_blindex;
}


static void send_all_trees(
    deflate_state *s,
    int lcodes, int dcodes, int blcodes )
{
    int rank;                    

    send_bits(s, lcodes-257, 5); 
    send_bits(s, dcodes-1,   5);
    send_bits(s, blcodes-4,  4); 
    for (rank = 0; rank < blcodes; rank++) {
        send_bits(s, s->bl_tree[bl_order[rank]].Len, 3);
    }

    send_tree(s, (ct_data *)s->dyn_ltree, lcodes-1); 
    send_tree(s, (ct_data *)s->dyn_dtree, dcodes-1); 
}


void _tr_stored_block(
    deflate_state *s,
    char *buf, 
    unsigned long stored_len,
    int last )
{
    send_bits(s, (STORED_BLOCK<<1)+last, 3);    
    copy_block(s, buf, (unsigned)stored_len, 1); 
}


void _tr_flush_bits(
    deflate_state *s )
{
    bi_flush(s);
}

void _tr_flush_block(
    deflate_state *s,
    char *buf, 
    unsigned long stored_len,
    int last )
{
    unsigned long opt_lenb, static_lenb; 
    int max_blindex = 0;  

	build_tree(s, (tree_desc *)(&(s->l_desc)));
	build_tree(s, (tree_desc *)(&(s->d_desc)));

	max_blindex = build_bl_tree(s);


	opt_lenb = (s->opt_len+3+7)>>3;
	static_lenb = (s->static_len+3+7)>>3;

	if (static_lenb <= opt_lenb) opt_lenb = static_lenb;
		
#ifdef FORCE_STORED
    if (buf != (char*)0) { 
#else
    if (stored_len+4 <= opt_lenb && buf != (char*)0) {
                       
#endif
        
        _tr_stored_block(s, buf, stored_len, last);

#ifdef FORCE_STATIC
    } else if (static_lenb >= 0) { 
#else
    } else if (static_lenb == opt_lenb) {
#endif
        send_bits(s, (STATIC_TREES<<1)+last, 3);
        compress_block(s, (ct_data *)static_ltree, (ct_data *)static_dtree);
    } else {
        send_bits(s, (DYN_TREES<<1)+last, 3);
        send_all_trees(s, s->l_desc.max_code+1, s->d_desc.max_code+1,
                       max_blindex+1);
        compress_block(s, (ct_data *)s->dyn_ltree, (ct_data *)s->dyn_dtree);
    }
    
    init_block(s);

    if (last) {
        bi_windup(s);
    }
}

static void compress_block(
    deflate_state *s,
    ct_data *ltree,
    ct_data *dtree ) 
{
    unsigned dist;      
    int lc;             
    unsigned lx = 0;    
    unsigned code;      
    int extra;          

    if (s->last_lit != 0) do {
        dist = s->d_buf[lx];
        lc = s->l_buf[lx++];
        if (dist == 0) {
            send_code(s, lc, ltree); 
        } else {
            
            code = _length_code[lc];
            send_code(s, code+LITERALS+1, ltree); 
            extra = extra_lbits[code];
            if (extra != 0) {
                lc -= base_length[code];
                send_bits(s, lc, extra);       
            }
            dist--; 
            code = d_code(dist);

            send_code(s, code, dtree);       
            extra = extra_dbits[code];
            if (extra != 0) {
                dist -= base_dist[code];
                send_bits(s, dist, extra);   
            }
        } 
    } while (lx < s->last_lit);

    send_code(s, END_BLOCK, ltree);
}


static unsigned bi_reverse(
    unsigned code,
    int len ) 
{
    register unsigned res = 0;
    do {
        res |= code & 1;
        code >>= 1, res <<= 1;
    } while (--len > 0);
    return res >> 1;
}


static void bi_flush(
    deflate_state *s )
{
    if (s->bi_valid == 16) {
        put_short(s, s->bi_buf);
        s->bi_buf = 0;
        s->bi_valid = 0;
    } else if (s->bi_valid >= 8) {
        put_byte(s, (Byte)s->bi_buf);
        s->bi_buf >>= 8;
        s->bi_valid -= 8;
    }
}


static void bi_windup(
    deflate_state *s )
{
    if (s->bi_valid > 8) {
        put_short(s, s->bi_buf);
    } else if (s->bi_valid > 0) {
        put_byte(s, (Byte)s->bi_buf);
    }
    s->bi_buf = 0;
    s->bi_valid = 0;
}


static void copy_block(
    deflate_state *s,
    char    *buf, 
    unsigned len,
    int      header )
{
    bi_windup(s);        

    if (header) {
        put_short(s, (unsigned short)len);
        put_short(s, (unsigned short)~len);
    }
    while (len--) {
        put_byte(s, *buf++);
    }
}
