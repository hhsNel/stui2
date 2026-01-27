#ifndef UTIL_H
#define UTIL_H

#include "stui2.h"

#include <stdint.h>
#include <stddef.h>

#define STUI_MB_EQ(BS,MASK) (((BS) & (MASK)) == (MASK))
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)((char *)__mptr - offsetof(type,member));})

#ifndef MIN
#define MIN(A,B) (((A) > (B)) ? (B) : (A))
#endif
#ifndef MAX
#define MAX(A,B) (((A) > (B)) ? (A) : (B))
#endif

typedef uint32_t data_len;

#endif

