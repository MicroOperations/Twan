#include <kernel/mem/mmu/paging.h>
#include <kernel/mem/mmu/tlb.h>
#include <lib/x86_index.h>
#include <kernel/kernel.h>
#include <subsys/sync/spinlock.h>
#include <kernel/extern.h>
#include <errno.h>
#include <std.h>
#include <kernel/kapi.h>
#include <multiboot2.h>
#include <subsys/mem/pma.h>
#include <subsys/mem/vma.h>

/* routines for vma remapping into kernel reserved vma descriptors (fine to use
   by initcalls and initialization code) */

static struct vma_descriptor pd_pfn_desc;
static struct vma_descriptor pd_pfn_io_desc;

static DEFINE_VMA_PARTITION(pd_pfn);
static DEFINE_VMA_PARTITION(pd_pfn_io);

REGISTER_VMA_PARTITION(pd_pfn, virt_to_phys_static(&pd_pfn), 
                       &pd_pfn_desc);

REGISTER_VMA_PARTITION(pd_pfn_io, virt_to_phys_static(&pd_pfn_io), 
                       &pd_pfn_io_desc);

static struct mapper __pd_pfn_mapper;
static struct mapper __pd_pfn_io_mapper;

void init_mappers(void)
{
    if (pd_pfn_desc.num_partitions_valid != 1)
        __early_kpanic("failed to initialize pd_pfn\n");

    if (pd_pfn_io_desc.num_partitions_valid != 1)
        __early_kpanic("failed to initialize pd_pfn_io\n");

    mcslock_isr_init(&__pd_pfn_mapper.lock);
    mcslock_isr_init(&__pd_pfn_io_mapper.lock);

    bmp512_set_all(&__pd_pfn_mapper.bmp);
    bmp512_set_all(&__pd_pfn_io_mapper.bmp);
}

/* pfn mappers */

void *__map_pfn_noflush(u64 pfn)
{
    int idx = bmp512_ffs_unset(&__pd_pfn_mapper.bmp); 
    if (idx == -1)
        return NULL;

    vma_partition_remap(&pd_pfn, idx, pfn);

    u64 addr = pd_pfn_desc.addr + (idx * PAGE_SIZE);
    return (void *)addr;
}

void *__map_pfn_local(u64 pfn)
{
    void *addr = __map_pfn_noflush(pfn);
    if (addr)
        flush_tlb_page_local((u64)addr);

    return addr;
}

void __unmap_pfn_noflush(void *addr)
{
    int idx = pd_index_of((u64)addr);
    vma_partition_unmap(&pd_pfn, idx);
    bmp512_set(&__pd_pfn_mapper.bmp, idx);
}

void __unmap_pfn_local(void *addr)
{
    __unmap_pfn_noflush(addr);
    flush_tlb_page_local((u64)addr);
}

void __remap_pfn_noflush(void *addr, u32 pfn)
{
    u32 idx = pd_index_of((u64)addr);
    vma_partition_remap(&pd_pfn, idx, pfn);
}

void __remap_pfn_local(void *addr, u32 pfn)
{
    __remap_pfn_noflush(addr, pfn);
    flush_tlb_page_local((u64)addr);
}

void *map_pfn(u64 pfn)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&__pd_pfn_mapper.lock, &node);
    void *addr = __map_pfn_noflush(pfn);
    mcs_unlock_isr_restore(&__pd_pfn_mapper.lock, &node);

    flush_tlb_page_global((u64)addr, true);
    return addr;
}

void unmap_pfn(void *addr)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&__pd_pfn_mapper.lock, &node);
    __unmap_pfn_noflush(addr);
    mcs_unlock_isr_restore(&__pd_pfn_mapper.lock, &node);

    flush_tlb_page_global((u64)addr, true);
}

void remap_pfn_local(void *addr, u32 pfn)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&__pd_pfn_mapper.lock, &node);
    __remap_pfn_local(addr, pfn);
    mcs_unlock_isr_restore(&__pd_pfn_mapper.lock, &node);
}

void remap_pfn(void *addr, u32 pfn)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&__pd_pfn_mapper.lock, &node);
    __remap_pfn_noflush(addr, pfn);
    mcs_unlock_isr_restore(&__pd_pfn_mapper.lock, &node);

    flush_tlb_page_global((u64)addr, true);
}

void *__map_phys_pg_noflush(u64 phys)
{
    u32 pfn = phys >> PAGE_SHIFT;
    u32 offset = phys % PAGE_SIZE;

    if (offset == 0)
        return __map_pfn_noflush(pfn);

    int idx = bmp512_ffs2_unset2(&__pd_pfn_mapper.bmp);
    if (idx == -1)
        return NULL;

    vma_partition_remap(&pd_pfn, idx, pfn);
    vma_partition_remap(&pd_pfn, idx + 1, pfn + 1);

    u64 addr = (pd_pfn_desc.addr + (idx * PAGE_SIZE)) + offset;
    return (void *)addr;
}

void *__map_phys_pg_local(u64 phys)
{
    void *addr = __map_phys_pg_noflush(phys);
    if (addr)
        flush_tlb_page_local((u64)addr);

    return addr;
}

void __unmap_phys_pg_noflush(void *addr)
{
    u8 *base = (void *)p2align_down((u64)addr, PAGE_SIZE);
    __unmap_pfn_noflush(base);

    u64 offset = (u64)addr % PAGE_SIZE;
    if (offset != 0) {
        u8 *stretch = base + PAGE_SIZE;
        __unmap_pfn_noflush(stretch);
    }
}

void __unmap_phys_pg_local(void *addr)
{
    __unmap_phys_pg_noflush(addr);
    flush_tlb_page_local((u64)addr);
}

void *map_phys_pg(u64 phys)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&__pd_pfn_mapper.lock, &node);
    void *addr = __map_phys_pg_noflush(phys);
    mcs_unlock_isr_restore(&__pd_pfn_mapper.lock, &node);
    
    if (addr) {

        flush_tlb_range_global(p2align_down((u64)addr, PAGE_SIZE), 
                               p2align_down((u64)addr + PAGE_SIZE, PAGE_SIZE));
    }

    return addr;
}

void unmap_phys_pg(void *addr)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&__pd_pfn_mapper.lock, &node);
    __unmap_phys_pg_noflush(addr);
    mcs_unlock_isr_restore(&__pd_pfn_mapper.lock, &node);

    flush_tlb_range_global(p2align_down((u64)addr, PAGE_SIZE), 
                           p2align_up((u64)addr, PAGE_SIZE));
}

void *__map_pfn_io_noflush(u64 pfn)
{
    int idx = bmp512_ffs_unset(&__pd_pfn_io_mapper.bmp);
    if (idx == -1)
        return NULL;

    vma_partition_remap_mmio(&pd_pfn_io, idx, pfn);

    u64 addr = pd_pfn_io_desc.addr + (idx * PAGE_SIZE);
    return (void *)addr;
}

void *__map_pfn_io_local(u64 pfn)
{
    void *addr = __map_pfn_io_noflush(pfn);
    if (addr)
        flush_tlb_page_local((u64)pfn << PAGE_SHIFT);

    return addr;
}

void __unmap_pfn_io_noflush(void *addr)
{
    int idx = pd_index_of((u64)addr);
    vma_partition_unmap(&pd_pfn_io, idx);
    bmp512_set(&__pd_pfn_io_mapper.bmp, idx);
}

void __unmap_pfn_io_local(void *addr)
{
    __unmap_pfn_io_noflush(addr);
    flush_tlb_page_local((u64)addr);
}

void *map_pfn_io(u64 pfn)
{
    struct mcsnode node = INITIALIZE_MCSNODE();
    
    mcs_lock_isr_save(&__pd_pfn_io_mapper.lock, &node);
    void *addr = __map_pfn_io_noflush(pfn);
    mcs_unlock_isr_restore(&__pd_pfn_io_mapper.lock, &node);

    if (addr)
        flush_tlb_page_global((u64)addr, true);

    return addr;
}

void unmap_pfn_io(void *addr)
{
    struct mcsnode node = INITIALIZE_MCSNODE();
    
    mcs_lock_isr_save(&__pd_pfn_io_mapper.lock, &node);
    __unmap_pfn_io_noflush(addr);
    mcs_unlock_isr_restore(&__pd_pfn_io_mapper.lock, &node);

    if (addr)
        flush_tlb_page_global((u64)addr, true);
}

void *__map_phys_pg_io_noflush(u64 phys)
{
    u32 pfn = phys >> PAGE_SHIFT;
    u32 offset = phys % PAGE_SIZE;

    if (offset == 0)
        return __map_pfn_io_noflush(pfn);

    int idx = bmp512_ffs2_unset2(&__pd_pfn_io_mapper.bmp);
    if (idx == -1)
        return NULL;

    vma_partition_remap_mmio(&pd_pfn_io, idx, pfn);
    vma_partition_remap_mmio(&pd_pfn_io, idx + 1, pfn + 1);

    u64 addr = (pd_pfn_io_desc.addr + (idx * PAGE_SIZE)) + offset;
    return (void *)addr;
   
}

void *__map_phys_pg_io_local(u64 phys)
{
    void *addr = __map_phys_pg_io_noflush(phys);
    if (addr) {
        flush_tlb_range_local(p2align_down((u64)addr, PAGE_SIZE), 
                              p2align_up((u64)addr, PAGE_SIZE));
    }

    return addr;
}

void __unmap_phys_pg_io_noflush(void *addr)
{
    u8 *base = (void *)p2align_down((u64)addr, PAGE_SIZE);
    __unmap_pfn_io_noflush(base);

    u64 offset = (u64)addr % PAGE_SIZE;
    if (offset != 0) {
         u8 *stretch = base + PAGE_SIZE;
        __unmap_pfn_io_noflush(stretch);
    }
}

void __unmap_phys_pg_io_local(void *addr)
{
    __unmap_phys_pg_io_noflush(addr);
    flush_tlb_range_local(p2align_down((u64)addr, PAGE_SIZE), 
                           p2align_up((u64)addr, PAGE_SIZE));
}

void *map_phys_pg_io(u64 phys)
{
    struct mcsnode node = INITIALIZE_MCSNODE();
    
    mcs_lock_isr_save(&__pd_pfn_io_mapper.lock, &node);
    void *addr = __map_phys_pg_io_noflush(phys);
    mcs_unlock_isr_restore(&__pd_pfn_io_mapper.lock, &node);
    
    if (addr) {

        flush_tlb_range_global(p2align_down((u64)addr, PAGE_SIZE), 
                               p2align_up((u64)addr, PAGE_SIZE));
    }

    return addr;
}

void unmap_phys_pg_io(void *addr)
{
    struct mcsnode node = INITIALIZE_MCSNODE();
    
    mcs_lock_isr_save(&__pd_pfn_io_mapper.lock, &node);
    __unmap_phys_pg_io_noflush(addr);
    mcs_unlock_isr_restore(&__pd_pfn_io_mapper.lock, &node);

    flush_tlb_range_global(p2align_down((u64)addr, PAGE_SIZE), 
                           p2align_up((u64)addr, PAGE_SIZE));
}