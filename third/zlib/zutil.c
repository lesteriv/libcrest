



#include "zutil.h"
#ifndef Z_SOLO
#  include "gzguts.h"
#endif

#ifndef NO_DUMMY_DECL
struct internal_state      {int dummy;}; 
#endif

const char * ZEXPORT zlibVersion()
{
    return ZLIB_VERSION;
}

#ifndef Z_SOLO

#ifdef SYS16BIT

#ifdef __TURBOC__


#  define MY_ZCALLOC



#define MAX_PTR 10


local int next_ptr = 0;

typedef struct ptr_table_s {
    voidpf org_ptr;
    voidpf new_ptr;
} ptr_table;

local ptr_table table[MAX_PTR];


voidpf ZLIB_INTERNAL zcalloc (voidpf opaque, unsigned items, unsigned size)
{
    voidpf buf = opaque; 
    ulg bsize = (ulg)items*size;

    
    if (bsize < 65520L) {
        buf = farmalloc(bsize);
        if (*(ush*)&buf != 0) return buf;
    } else {
        buf = farmalloc(bsize + 16L);
    }
    if (buf == NULL || next_ptr >= MAX_PTR) return NULL;
    table[next_ptr].org_ptr = buf;

    
    *((ush*)&buf+1) += ((ush)((uch*)buf-0) + 15) >> 4;
    *(ush*)&buf = 0;
    table[next_ptr++].new_ptr = buf;
    return buf;
}

void ZLIB_INTERNAL zcfree (voidpf opaque, voidpf ptr)
{
    int n;
    if (*(ush*)&ptr != 0) { 
        farfree(ptr);
        return;
    }
    
    for (n = 0; n < next_ptr; n++) {
        if (ptr != table[n].new_ptr) continue;

        farfree(table[n].org_ptr);
        while (++n < next_ptr) {
            table[n-1] = table[n];
        }
        next_ptr--;
        return;
    }
    ptr = opaque; 
    Assert(0, "zcfree: ptr not found");
}

#endif 


#ifdef M_I86


#  define MY_ZCALLOC

#if (!defined(_MSC_VER) || (_MSC_VER <= 600))
#  define _halloc  halloc
#  define _hfree   hfree
#endif

voidpf ZLIB_INTERNAL zcalloc (voidpf opaque, uInt items, uInt size)
{
    if (opaque) opaque = 0; 
    return _halloc((long)items, size);
}

void ZLIB_INTERNAL zcfree (voidpf opaque, voidpf ptr)
{
    if (opaque) opaque = 0; 
    _hfree(ptr);
}

#endif 

#endif 


#ifndef MY_ZCALLOC 

#ifndef STDC
extern voidp  malloc OF((uInt size));
extern voidp  calloc OF((uInt items, uInt size));
extern void   free   OF((voidpf ptr));
#endif

voidpf ZLIB_INTERNAL zcalloc (opaque, items, size)
    voidpf opaque;
    unsigned items;
    unsigned size;
{
    if (opaque) items += size - size; 
    return sizeof(uInt) > 2 ? (voidpf)malloc(items * size) :
                              (voidpf)calloc(items, size);
}

void ZLIB_INTERNAL zcfree (opaque, ptr)
    voidpf opaque;
    voidpf ptr;
{
    free(ptr);
    if (opaque) return; 
}

#endif 

#endif 
