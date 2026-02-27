#ifndef _RTALLOC_H_
#define _RTALLOC_H_

#include <std.h>

typedef void *(*rtalloc_t)(size_t size);
typedef void (*rtfree_t)(void *addr);
typedef void *(*rtrealloc_t)(void *addr, size_t size);
typedef void *(*rtallocz_t)(size_t size);

typedef void *(*rtalloc_p2aligned_t)(size_t size, u32 alignment);
typedef void (*rtfree_p2aligned_t)(void *addr);
typedef void *(*rtrealloc_p2aligned_t)(void *addr, size_t size, u32 alignment);
typedef void *(*rtallocz_p2aligned_t)(size_t size, u32 alignment);

struct rtalloc_interface
{
    rtalloc_t rtalloc_func;
    rtfree_t rtfree_func;
    rtrealloc_t rtrealloc_func;

    rtalloc_p2aligned_t rtalloc_p2aligned_func;
    rtfree_p2aligned_t rtfree_p2aligned_func;
    rtrealloc_p2aligned_t rtrealloc_p2aligned_func;
};

struct rtalloc
{
    struct rtalloc_interface *interface;
    bool initialized;
};

int rtalloc_init(struct rtalloc_interface *interface);

bool is_rtalloc_initialized(void);

void *rtalloc(size_t size);
void rtfree(void *addr);
void *rtrealloc(void *addr, size_t size);
void *rtallocz(size_t size);

/* p2aligned routines must only be used in combination with other p2aligned 
   routines !! */

void *rtalloc_p2aligned(size_t size, u32 alignment);
void rtfree_p2aligned(void *addr);
void *rtrealloc_p2aligned(void *addr, size_t size, u32 alignment);
void *rtallocz_p2aligned(size_t size, u32 alignment);

#endif