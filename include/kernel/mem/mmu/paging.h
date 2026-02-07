#ifndef _PAGING_H_
#define _PAGING_H_

#include <include/kernel/kernel.h>
#include <include/lib/dsa/bmp512.h>
#include <include/subsys/sync/mcslock.h>

struct mapper
{
    struct bmp512 bmp;
    struct mcslock_isr lock;
};

void init_mappers(void);

/* mapping specific pfns */

void *__map_pfn_noflush(u64 pfn);
void *__map_pfn_local(u64 pfn);

void __unmap_pfn_noflush(void *addr);
void __unmap_pfn_local(void *addr);

void *map_pfn(u64 pfn);
void unmap_pfn(void *addr);

void *__map_phys_pg_noflush(u64 phys);
void *__map_phys_pg_local(u64 phys);

void __unmap_phys_pg_noflush(void *addr);
void __unmap_phys_pg_local(void *addr);

void *map_phys_pg(u64 phys);
void unmap_phys_pg(void *addr);

/* mapping io memory */

void *__map_pfn_io_noflush(u64 pfn);
void *__map_pfn_io_local(u64 pfn);

void __unmap_pfn_io_noflush(void *addr);
void __unmap_pfn_io_local(void *addr);

void __remap_pfn_noflush(void *addr, u32 pfn);
void __remap_pfn_local(void *addr, u32 pfn);

void *map_pfn_io(u64 pfn);
void unmap_pfn_io(void *addr);
void remap_pfn_local(void *addr, u32 pfn);
void remap_pfn(void *addr, u32 pfn);

void *__map_phys_pg_io_noflush(u64 phys);
void *__map_phys_pg_io_local(u64 phys);

void __unmap_phys_pg_io_noflush(void *addr);
void __unmap_phys_pg_io_local(void *addr);

void *map_phys_pg_io(u64 phys);
void unmap_phys_pg_io(void *addr);

#endif