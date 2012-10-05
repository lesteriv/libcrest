/**********************************************************************************************/
/* deflate.cpp		  		                                                   				  */
/*                                                                       					  */
/* (C) 1995-2012 Jean-loup Gailly and Mark Adler											  */
/* ZLIB license   																		  	  */
/**********************************************************************************************/

// STD
#include <stdlib.h>
#include <string.h>

// ZLIB
#include "deflate.h"


#define BASE 65521      /* largest prime smaller than 65536 */
#define NMAX 5552		/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define Z_DEFLATED   8

#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

#define MOD(a) a %= BASE
#define MOD28(a) a %= BASE

unsigned long adler32(
    unsigned long adler,
    const Byte *buf,
    unsigned int len )
{
    unsigned long sum2;
    unsigned n;

    /* split Adler-32 into component sums */
    sum2 = (adler >> 16) & 0xffff;
    adler &= 0xffff;

    /* in case user likes doing a byte at a time, keep it fast */
    if (len == 1) {
        adler += buf[0];
        if (adler >= BASE)
            adler -= BASE;
        sum2 += adler;
        if (sum2 >= BASE)
            sum2 -= BASE;
        return adler | (sum2 << 16);
    }

    /* initial Adler-32 value (deferred check for len == 1 speed) */
    if (buf == 0)
        return 1L;

    /* in case short lengths are provided, keep it somewhat fast */
    if (len < 16) {
        while (len--) {
            adler += *buf++;
            sum2 += adler;
        }
        if (adler >= BASE)
            adler -= BASE;
        MOD28(sum2);            /* only added so many BASE's */
        return adler | (sum2 << 16);
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while (len >= NMAX) {
        len -= NMAX;
        n = NMAX / 16;          /* NMAX is divisible by 16 */
        do {
            DO16(buf);          /* 16 sums unrolled */
            buf += 16;
        } while (--n);
        MOD(adler);
        MOD(sum2);
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if (len) {                  /* avoid modulos if none remaining */
        while (len >= 16) {
            len -= 16;
            DO16(buf);
            buf += 16;
        }
        while (len--) {
            adler += *buf++;
            sum2 += adler;
        }
        MOD(adler);
        MOD(sum2);
    }

    /* return recombined sums */
    return adler | (sum2 << 16);
}


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
static void putShortMSB    (deflate_state *s, unsigned int b);
static void flush_pending  (z_stream* strm);
static int read_buf        (z_stream* strm, Byte *buf, unsigned size);
static unsigned int longest_match  (deflate_state *s, IPos cur_match);

#define NIL 0

typedef struct config_s {
   unsigned short good_length; 
   unsigned short max_lazy;    
   unsigned short nice_length; 
   unsigned short max_chain;
   compress_func func;
} config;


#define UPDATE_HASH(s,h,c) (h = (((h)<<s->hash_shift) ^ (c)) & s->hash_mask)

#define INSERT_STRING(s, str, match_head) \
   (UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
    match_head = s->head[s->ins_h], \
    s->head[s->ins_h] = (Pos)(str))

#define CLEAR_HASH(s) \
    s->head[s->hash_size-1] = NIL; \
    memset(s->head, 0, (unsigned)(s->hash_size-1)*sizeof(*s->head));

static void deflateResetKeep(
    z_stream* strm )
{
    deflate_state *s;

    s = (deflate_state *)strm->state;
    s->pending = 0;
    s->pending_out = s->pending_buf;

    if (s->wrap < 0) {
        s->wrap = -s->wrap; 
    }
    s->status = s->wrap ? INIT_STATE : BUSY_STATE;
    strm->adler =
        adler32(0L, 0, 0);

    _tr_init(s);
}

static void deflateReset(
    z_stream* strm )
{
    deflateResetKeep(strm);
    lm_init(strm->state);
}
	
void deflateInit( z_stream* strm )
{
    deflate_state *s;
    int wrap = 1;

    unsigned short *overlay;

    s = (deflate_state *) malloc(sizeof(deflate_state));
    strm->state = (struct internal_state *)s;
    s->strm = strm;

    s->wrap = wrap;
    s->w_bits = 15;
    s->w_size = 1 << s->w_bits;

    s->hash_bits = 15;
    s->hash_size = 1 << s->hash_bits;
    s->hash_mask = s->hash_size - 1;
    s->hash_shift =  ((s->hash_bits+MIN_MATCH-1)/MIN_MATCH);

    s->window = (Byte *) malloc(s->w_size * 2*sizeof(Byte));
    s->prev   = (Posf *) malloc(s->w_size * sizeof(Pos));
    s->head   = (Posf *) malloc(s->hash_size * sizeof(Pos));

    s->high_water = 0;      

    s->lit_bufsize = 1 << (8 + 6); 

    overlay = (unsigned short*) malloc(s->lit_bufsize * (sizeof(unsigned short)+2));
    s->pending_buf = (unsigned char *) overlay;
    s->pending_buf_size = (unsigned long)s->lit_bufsize * (sizeof(unsigned short)+2L);
    s->d_buf = overlay + s->lit_bufsize/sizeof(unsigned short);
    s->l_buf = s->pending_buf + (1+sizeof(unsigned short))*s->lit_bufsize;

    deflateReset(strm);
}

static void putShortMSB(
    deflate_state *s,
    unsigned int b )
{
    put_byte(s, (Byte)(b >> 8));
    put_byte(s, (Byte)(b & 0xff));
}


static void flush_pending(
    z_stream* strm )
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

void deflate (
    z_stream* strm )
{
	deflateInit( strm );
	
    deflate_state *s;

    s = strm->state;
    s->strm = strm; 
    
    if (s->status == INIT_STATE)
	{
		unsigned int header = (Z_DEFLATED + ((s->w_bits-8)<<4)) << 8;
		if (s->strstart != 0) header |= 0x20;
		header += 31 - (header % 31);

		s->status = BUSY_STATE;
		putShortMSB(s, header);

		if (s->strstart != 0) {
			putShortMSB(s, (unsigned int)(strm->adler >> 16));
			putShortMSB(s, (unsigned int)(strm->adler & 0xffff));
		}
		strm->adler = adler32(0L, 0, 0);
	}
    
    if (s->pending != 0) {
        flush_pending(strm);
        if (strm->avail_out == 0) {
            goto finish;
        }
    }
    
    if (strm->avail_in != 0 || s->lookahead != 0 ||
        s->status != FINISH_STATE) {
        block_state bstate;

        bstate = deflate_fast(s, 4);

        if (bstate == finish_started || bstate == finish_done) {
            s->status = FINISH_STATE;
        }
        if (bstate == need_more || bstate == finish_started) {
            goto finish;
        }
        if (bstate == block_done) {
			_tr_stored_block(s, (char*)0, 0L, 0);

            flush_pending(strm);
            if (strm->avail_out == 0) {
              goto finish;
            }
        }
    }

	putShortMSB(s, (unsigned int)(strm->adler >> 16));
	putShortMSB(s, (unsigned int)(strm->adler & 0xffff));
    flush_pending(strm);
    
finish:
    free(strm->state->pending_buf);
    free(strm->state->head);
    free(strm->state->prev);
    free(strm->state->window);
    free(strm->state);
}

static int read_buf(
    z_stream* strm,
    Byte *buf,
    unsigned size )
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


static void lm_init (
    deflate_state *s )
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

static unsigned int longest_match(
    deflate_state *s,
    IPos cur_match )                           
{
    register Byte *scan = s->window + s->strstart; 
    register Byte *match;                       
    register int len;                           
    register Byte *strend = s->window + s->strstart + MAX_MATCH;

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
    return (unsigned int)len <= s->lookahead ? (unsigned int)len : s->lookahead;
}

static void fill_window(
    deflate_state *s )
{
    register unsigned n, m;
    register Posf *p;
    unsigned more;    
    unsigned int wsize = s->w_size;

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
            unsigned int str = s->strstart - s->insert;
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
                   (char *)&s->window[(unsigned)s->block_start] : \
                   (char *)0), \
                (unsigned long)((long)s->strstart - s->block_start), \
                (last)); \
   s->block_start = s->strstart; \
   flush_pending(s->strm); \
}


#define FLUSH_BLOCK(s, last) { \
   FLUSH_BLOCK_ONLY(s, last); \
   if (s->strm->avail_out == 0) return (last) ? finish_started : need_more; \
}


static block_state deflate_fast(
    deflate_state *s,
    int flush )
{
    IPos hash_head;       
    int bflush;           

    for (;;) {
        
        if (s->lookahead < MIN_LOOKAHEAD) {
            fill_window(s);
            if (s->lookahead < MIN_LOOKAHEAD && flush == 0) {
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
    if (flush == 4) {
        FLUSH_BLOCK(s, 1);
        return finish_done;
    }
    if (s->last_lit)
        FLUSH_BLOCK(s, 0);
    return block_done;
}
