#include "deflate.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
    need_more,      
    block_done,     
    finish_started, 
    finish_done     
} block_state;

typedef block_state (*compress_func) (deflate_state *s, int flush);

static void fill_window    (deflate_state *s);
static block_state deflate_fast   (deflate_state *s, int flush);
static void lm_init        (deflate_state *s);
static void putShortMSB    (deflate_state *s, uInt b);
static void flush_pending  (z_streamp strm);
static int read_buf        (z_streamp strm, Bytef *buf, unsigned size);
static uInt longest_match  (deflate_state *s, IPos cur_match);

#define NIL 0

#ifndef TOO_FAR
#  define TOO_FAR 4096
#endif


typedef struct config_s {
   unsigned short good_length; 
   unsigned short max_lazy;    
   unsigned short nice_length; 
   unsigned short max_chain;
   compress_func func;
} config;


#define RANK(f) (((f) << 1) - ((f) > 4 ? 9 : 0))
#define UPDATE_HASH(s,h,c) (h = (((h)<<s->hash_shift) ^ (c)) & s->hash_mask)

#define INSERT_STRING(s, str, match_head) \
   (UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
    match_head = s->head[s->ins_h], \
    s->head[s->ins_h] = (Pos)(str))

#define CLEAR_HASH(s) \
    s->head[s->hash_size-1] = NIL; \
    memset(s->head, 0, (unsigned)(s->hash_size-1)*sizeof(*s->head));

static int deflateResetKeep (strm)
    z_streamp strm;
{
    deflate_state *s;

    if (strm == Z_NULL || strm->state == Z_NULL ) {
        return Z_STREAM_ERROR;
    }

    s = (deflate_state *)strm->state;
    s->pending = 0;
    s->pending_out = s->pending_buf;

    if (s->wrap < 0) {
        s->wrap = -s->wrap; 
    }
    s->status = s->wrap ? INIT_STATE : BUSY_STATE;
    strm->adler =
        adler32(0L, Z_NULL, 0);

    _tr_init(s);

    return Z_OK;
}

static int deflateReset (strm)
    z_streamp strm;
{
    int ret;

    ret = deflateResetKeep(strm);
    if (ret == Z_OK)
        lm_init(strm->state);
    return ret;
}
	
static int deflateInit2_(strm, method, windowBits, memLevel,
                  version, stream_size)
    z_streamp strm;
    int  method;
    int  windowBits;
    int  memLevel;
    const char *version;
    int stream_size;
{
    deflate_state *s;
    int wrap = 1;
    static const char my_version[] = ZLIB_VERSION;

    unsigned short *overlay;
    

    if (version == Z_NULL || version[0] != my_version[0] ||
        stream_size != sizeof(z_stream)) {
        return Z_VERSION_ERROR;
    }
    if (strm == Z_NULL) return Z_STREAM_ERROR;

    if (windowBits < 0) { 
        wrap = 0;
        windowBits = -windowBits;
    }
    if (memLevel < 1 || memLevel > MAX_MEM_LEVEL || method != Z_DEFLATED ||
        windowBits < 8 || windowBits > 15) {
        return Z_STREAM_ERROR;
    }
    if (windowBits == 8) windowBits = 9;  
    s = (deflate_state *) malloc(sizeof(deflate_state));
    if (s == Z_NULL) return Z_MEM_ERROR;
    strm->state = (struct internal_state *)s;
    s->strm = strm;

    s->wrap = wrap;
    s->w_bits = windowBits;
    s->w_size = 1 << s->w_bits;

    s->hash_bits = memLevel + 7;
    s->hash_size = 1 << s->hash_bits;
    s->hash_mask = s->hash_size - 1;
    s->hash_shift =  ((s->hash_bits+MIN_MATCH-1)/MIN_MATCH);

    s->window = (Bytef *) malloc(s->w_size * 2*sizeof(Byte));
    s->prev   = (Posf *)  malloc(s->w_size * sizeof(Pos));
    s->head   = (Posf *)  malloc(s->hash_size * sizeof(Pos));

    s->high_water = 0;      

    s->lit_bufsize = 1 << (memLevel + 6); 

    overlay = (unsigned short *) malloc(s->lit_bufsize * sizeof(unsigned short)+2);
    s->pending_buf = (unsigned char *) overlay;
    s->pending_buf_size = (unsigned long)s->lit_bufsize * (sizeof(unsigned short)+2L);

    if (s->window == Z_NULL || s->prev == Z_NULL || s->head == Z_NULL ||
        s->pending_buf == Z_NULL) {
        s->status = FINISH_STATE;
        deflateEnd (strm);
        return Z_MEM_ERROR;
    }
    s->d_buf = overlay + s->lit_bufsize/sizeof(unsigned short);
    s->l_buf = s->pending_buf + (1+sizeof(unsigned short))*s->lit_bufsize;

    return deflateReset(strm);
}

int deflateInit_(strm, version, stream_size)
    z_streamp strm;
    const char *version;
    int stream_size;
{
    return deflateInit2_(strm, Z_DEFLATED, MAX_WBITS, 8,
                         version, stream_size);
    
}

static void putShortMSB (s, b)
    deflate_state *s;
    uInt b;
{
    put_byte(s, (Byte)(b >> 8));
    put_byte(s, (Byte)(b & 0xff));
}


static void flush_pending(strm)
    z_streamp strm;
{
    unsigned len;
    deflate_state *s = strm->state;

    _tr_flush_bits(s);
    len = s->pending;
    if (len > strm->avail_out) len = strm->avail_out;
    if (len == 0) return;

    memcpy(strm->next_out, s->pending_out, len);
    strm->next_out  += len;
    s->pending_out  += len;
    strm->avail_out  -= len;
    s->pending -= len;
    if (s->pending == 0) {
        s->pending_out = s->pending_buf;
    }
}


int deflate (strm)
    z_streamp strm;
{
    deflate_state *s;

    if (strm == Z_NULL || strm->state == Z_NULL) {
        return Z_STREAM_ERROR;
    }
    s = strm->state;

    if (strm->next_out == Z_NULL ||
        (strm->next_in == Z_NULL && strm->avail_in != 0)) {
        return Z_STREAM_ERROR;
    }
    if (strm->avail_out == 0) return Z_BUF_ERROR;

    s->strm = strm; 
    
    if (s->status == INIT_STATE) {
        {
            uInt header = (Z_DEFLATED + ((s->w_bits-8)<<4)) << 8;
            if (s->strstart != 0) header |= 0x20;
            header += 31 - (header % 31);

            s->status = BUSY_STATE;
            putShortMSB(s, header);

            
            if (s->strstart != 0) {
                putShortMSB(s, (uInt)(strm->adler >> 16));
                putShortMSB(s, (uInt)(strm->adler & 0xffff));
            }
            strm->adler = adler32(0L, Z_NULL, 0);
        }
    }
    
    if (s->pending != 0) {
        flush_pending(strm);
        if (strm->avail_out == 0) {
            return Z_OK;
        }
    }
    
    if (s->status == FINISH_STATE && strm->avail_in != 0) {
        return Z_BUF_ERROR;
    }

    
    if (strm->avail_in != 0 || s->lookahead != 0 ||
        s->status != FINISH_STATE) {
        block_state bstate;

        bstate = deflate_fast(s, Z_FINISH);

        if (bstate == finish_started || bstate == finish_done) {
            s->status = FINISH_STATE;
        }
        if (bstate == need_more || bstate == finish_started) {
            return Z_OK;
        }
        if (bstate == block_done) {
			_tr_stored_block(s, (char*)0, 0L, 0);

            flush_pending(strm);
            if (strm->avail_out == 0) {
              return Z_OK;
            }
        }
    }

    if (s->wrap <= 0) return Z_STREAM_END;

    {
        putShortMSB(s, (uInt)(strm->adler >> 16));
        putShortMSB(s, (uInt)(strm->adler & 0xffff));
    }
    flush_pending(strm);
    
    if (s->wrap > 0) s->wrap = -s->wrap; 
    return s->pending != 0 ? Z_OK : Z_STREAM_END;
}


int deflateEnd (strm)
    z_streamp strm;
{
    int status;

    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;

    status = strm->state->status;
    if (status != INIT_STATE &&
        status != EXTRA_STATE &&
        status != NAME_STATE &&
        status != COMMENT_STATE &&
        status != HCRC_STATE &&
        status != BUSY_STATE &&
        status != FINISH_STATE) {
      return Z_STREAM_ERROR;
    }
    
    free(strm->state->pending_buf);
    free(strm->state->head);
    free(strm->state->prev);
    free(strm->state->window);

    free(strm->state);
    strm->state = Z_NULL;

    return status == BUSY_STATE ? Z_DATA_ERROR : Z_OK;
}

static int read_buf(strm, buf, size)
    z_streamp strm;
    Bytef *buf;
    unsigned size;
{
    unsigned len = strm->avail_in;

    if (len > size) len = size;
    if (len == 0) return 0;

    strm->avail_in  -= len;

    memcpy(buf, strm->next_in, len);
    if (strm->state->wrap == 1) {
        strm->adler = adler32(strm->adler, buf, len);
    }
    strm->next_in  += len;

    return (int)len;
}


static void lm_init (s)
    deflate_state *s;
{
    s->window_size = (unsigned long)2L*s->w_size;

    CLEAR_HASH(s);

    s->strstart = 0;
    s->block_start = 0L;
    s->lookahead = 0;
    s->insert = 0;
    s->match_length = MIN_MATCH-1;
    s->ins_h = 0;
}

#define check_match(s, start, match, length)

static uInt longest_match(s, cur_match)
    deflate_state *s;
    IPos cur_match;                             
{
    register Bytef *scan = s->window + s->strstart; 
    register Bytef *match;                       
    register int len;                           
    register Bytef *strend = s->window + s->strstart + MAX_MATCH;

    match = s->window + cur_match;

    
    if (match[0] != scan[0] || match[1] != scan[1]) return MIN_MATCH-1;

    
    scan += 2, match += 2;
    
    do {
    } while (*++scan == *++match && *++scan == *++match &&
             *++scan == *++match && *++scan == *++match &&
             *++scan == *++match && *++scan == *++match &&
             *++scan == *++match && *++scan == *++match &&
             scan < strend);

    len = MAX_MATCH - (int)(strend - scan);

    if (len < MIN_MATCH) return MIN_MATCH - 1;

    s->match_start = cur_match;
    return (uInt)len <= s->lookahead ? (uInt)len : s->lookahead;
}

static void fill_window(s)
    deflate_state *s;
{
    register unsigned n, m;
    register Posf *p;
    unsigned more;    
    uInt wsize = s->w_size;

    do {
        more = (unsigned)(s->window_size -(unsigned long)s->lookahead -(unsigned long)s->strstart);

        
        if (sizeof(int) <= 2) {
            if (more == 0 && s->strstart == 0 && s->lookahead == 0) {
                more = wsize;

            } else if (more == (unsigned)(-1)) {
                
                more--;
            }
        }

        
        if (s->strstart >= wsize+MAX_DIST(s)) {

            memcpy(s->window, s->window+wsize, (unsigned)wsize);
            s->match_start -= wsize;
            s->strstart    -= wsize; 
            s->block_start -= (long) wsize;

            
            n = s->hash_size;
            p = &s->head[n];
            do {
                m = *--p;
                *p = (Pos)(m >= wsize ? m-wsize : NIL);
            } while (--n);

            n = wsize;
            more += wsize;
        }
        if (s->strm->avail_in == 0) break;

        n = read_buf(s->strm, s->window + s->strstart + s->lookahead, more);
        s->lookahead += n;

        
        if (s->lookahead + s->insert >= MIN_MATCH) {
            uInt str = s->strstart - s->insert;
            s->ins_h = s->window[str];
            UPDATE_HASH(s, s->ins_h, s->window[str + 1]);
            while (s->insert) {
                UPDATE_HASH(s, s->ins_h, s->window[str + MIN_MATCH-1]);
                s->head[s->ins_h] = (Pos)str;
                str++;
                s->insert--;
                if (s->lookahead + s->insert < MIN_MATCH)
                    break;
            }
        }
        

    } while (s->lookahead < MIN_LOOKAHEAD && s->strm->avail_in != 0);

    
    if (s->high_water < s->window_size) {
        unsigned long curr = s->strstart + (unsigned long)(s->lookahead);
        unsigned long init;

        if (s->high_water < curr) {
            
            init = s->window_size - curr;
            if (init > WIN_INIT)
                init = WIN_INIT;
            memset(s->window + curr, 0, (unsigned)init);
            s->high_water = curr + init;
        }
        else if (s->high_water < (unsigned long)curr + WIN_INIT) {
            
            init = (unsigned long)curr + WIN_INIT - s->high_water;
            if (init > s->window_size - s->high_water)
                init = s->window_size - s->high_water;
            memset(s->window + s->high_water, 0, (unsigned)init);
            s->high_water += init;
        }
    }
}


#define FLUSH_BLOCK_ONLY(s, last) { \
   _tr_flush_block(s, (s->block_start >= 0L ? \
                   (charf *)&s->window[(unsigned)s->block_start] : \
                   (charf *)Z_NULL), \
                (unsigned long)((long)s->strstart - s->block_start), \
                (last)); \
   s->block_start = s->strstart; \
   flush_pending(s->strm); \
}


#define FLUSH_BLOCK(s, last) { \
   FLUSH_BLOCK_ONLY(s, last); \
   if (s->strm->avail_out == 0) return (last) ? finish_started : need_more; \
}


static block_state deflate_fast(s, flush)
    deflate_state *s;
    int flush;
{
    IPos hash_head;       
    int bflush;           

    for (;;) {
        
        if (s->lookahead < MIN_LOOKAHEAD) {
            fill_window(s);
            if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) {
                return need_more;
            }
            if (s->lookahead == 0) break; 
        }

        
        hash_head = NIL;
        if (s->lookahead >= MIN_MATCH) {
            INSERT_STRING(s, s->strstart, hash_head);
        }

        
        if (hash_head != NIL && s->strstart - hash_head <= MAX_DIST(s)) {
            
            s->match_length = longest_match (s, hash_head);
            
        }
        if (s->match_length >= MIN_MATCH) {
            check_match(s, s->strstart, s->match_start, s->match_length);

            _tr_tally_dist(s, s->strstart - s->match_start,
                           s->match_length - MIN_MATCH, bflush);

            s->lookahead -= s->match_length;

            {
                s->strstart += s->match_length;
                s->match_length = 0;
                s->ins_h = s->window[s->strstart];
                UPDATE_HASH(s, s->ins_h, s->window[s->strstart+1]);
            }
        } else {
            
            _tr_tally_lit (s, s->window[s->strstart], bflush);
            s->lookahead--;
            s->strstart++;
        }
        if (bflush) FLUSH_BLOCK(s, 0);
    }
    s->insert = s->strstart < MIN_MATCH-1 ? s->strstart : MIN_MATCH-1;
    if (flush == Z_FINISH) {
        FLUSH_BLOCK(s, 1);
        return finish_done;
    }
    if (s->last_lit)
        FLUSH_BLOCK(s, 0);
    return block_done;
}
