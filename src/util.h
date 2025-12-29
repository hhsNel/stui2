#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define STUI_OK  0
#define STUI_ERR 1

#define STUI_MB_EQ(BS,MASK) (((BS) & (MASK)) == (MASK))
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)((char *)__mptr - offsetof(type,member));})

typedef uint32_t data_len;

#endif

