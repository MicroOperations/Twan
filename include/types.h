#ifndef _TYPES_H_
#define _TYPES_H_

#include <compiler.h>

#if !VERSION_C23

    typedef int bool;
    #define true 1
    #define false 0
    
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
    #define _Bool int
#endif


#ifndef NULL
#define NULL 0
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef long unsigned int size_t;

#endif