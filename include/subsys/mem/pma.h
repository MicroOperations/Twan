#ifndef _PMA_H_
#define _PMA_H_

#include <include/std.h>

#define PMA_BASE_ORDER PAGE_SHIFT
#define PMA_NULL 0

typedef int (*pma_init_range_t)(u64 phys, size_t size);
typedef u64 (*pma_alloc_pages_t)(u32 num_pages, void **id);
typedef int (*pma_free_pages_t)(void *id, u64 phys, u32 num_pages);

struct pma_interface
{
    pma_init_range_t pma_init_range_func;
    pma_alloc_pages_t pma_alloc_pages_func;
    pma_free_pages_t pma_free_pages_func;
};

struct pma
{
    struct pma_interface *interface;
    bool initialized;
};

int pma_init(struct pma_interface *interface);

bool is_pma_initialized(void);

int pma_init_range(u64 phys, size_t size);
u64 pma_alloc_pages(u32 num_pages, void **id);
int pma_free_pages(void *id, u64 phys, u32 num_pages);

#endif