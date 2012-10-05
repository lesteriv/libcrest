



#include "zutil.h"
#ifndef Z_SOLO
#  include "gzguts.h"
#endif

#ifndef NO_DUMMY_DECL
struct internal_state      {int dummy;}; 
#endif

#ifndef Z_SOLO

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
