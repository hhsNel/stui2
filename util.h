#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define STUI_OK  0
#define STUI_ERR 1

#define STUI_MB_EQ(BS,MASK) (((BS) & (MASK)) == (MASK))

typedef uint32_t data_len;

#endif

