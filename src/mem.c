/* freestanding mem funcs — gcc emits calls for struct copies */
#include "gba.h"

void *memcpy(void *dst, const void *src, unsigned n)
{
    u8 *d = dst; const u8 *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, unsigned n)
{
    u8 *d = dst; const u8 *s = src;
    if (d < s) { while (n--) *d++ = *s++; }
    else { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}

void *memset(void *dst, int c, unsigned n)
{
    u8 *d = dst;
    while (n--) *d++ = (u8)c;
    return dst;
}
