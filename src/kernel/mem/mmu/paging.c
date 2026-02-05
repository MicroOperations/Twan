#include <include/kernel/mem/mmu/paging.h>
#include <include/kernel/mem/mmu/tlb.h>
#include <include/lib/x86_index.h>
#include <include/kernel/kernel.h>
#include <include/subsys/sync/spinlock.h>
#include <include/kernel/extern.h>
#include <include/errno.h>
#include <include/std.h>
#include <include/kernel/kapi.h>
#include <include/kernel/mem/mem_conf.h>
#include <include/multiboot2.h>
#include <include/subsys/mem/pma.h>

static struct mapper __pd_pfn_mapper;
static struct mapper __pd_pfn_io_mapper;

static struct mcslock_isr keyhole_locks[min(NUM_CPUS, 512)];

void *__pt_walk(va_t va, int *level)
{
    *level = PGTABLE_LEVEL_NONE;

    u32 cpu = this_processor_id();
    u32 idx = cpu % ARRAY_LEN(keyhole_locks);
    u8 *keyhole = (void *)((u64)PD_KEYHOLES_START_ADDR | 
                           (idx << PAGE_SHIFT_2MB));

    pml4e_t pml4e = pml4[va.fields.pml4];
    if (pml4e.fields.present == 0)
        return NULL;

    /* remap to pdpt */

    u64 pdpt_phys = pml4e.fields.pdpt_phys << PAGE_SHIFT_4KB;
    u32 pdpt_pfn = pdpt_phys >> PAGE_SHIFT;
    u32 pdpt_offset = pdpt_phys % PAGE_SIZE;
    __remap_keyhole_local(keyhole, pdpt_pfn);

    pdpte_t *__pdpt = (void *)(keyhole + pdpt_offset); 
    pdpte_t *pdpte = &__pdpt[va.fields.pdpt];

    if (pdpte->fields.present == 0)
        return NULL;

    if (pdpte->fields.ps != 0) {
        *level = PGTABLE_LEVEL_1GB;
        return pdpte;
    }

    /* remap to pd */

    u64 pd_phys = pdpte->fields.pd_phys << PAGE_SHIFT_4KB;
    u32 pd_pfn = pd_phys >> PAGE_SHIFT;
    u32 pd_offset = pd_phys % PAGE_SIZE;
    __remap_keyhole_local(keyhole, pd_pfn);

    pde_t *__pd = (void *)(keyhole + pd_offset);
    pde_t *pde = &__pd[va.fields.pd];

    if (pde->fields.present == 0)
        return NULL;

    if (pde->fields.ps != 0) {
        *level = PGTABLE_LEVEL_2MB;
        return pde;
    }

    /* remap to pt */

    u64 pt_phys = pde->fields.pt_phys << PAGE_SHIFT_4KB;
    u32 pt_pfn = pt_phys >> PAGE_SHIFT;
    u32 pt_offset = pt_phys % PAGE_SIZE;
    __remap_keyhole_local(keyhole, pt_pfn);

    pte_t *__pt = (void *)(keyhole + pt_offset);
    pte_t *pte = &__pt[pt_index_of(va.val)];

    if (pte->fields.present == 0)
        return NULL;

    *level = PGTABLE_LEVEL_4K;
    return pte;
}

void *pt_walk(va_t va, int *level)
{
    int __level = PGTABLE_LEVEL_NONE;
    void *entry = NULL;

    u32 cpu = this_processor_id();

    struct mcslock_isr *lock = &keyhole_locks[cpu % ARRAY_LEN(keyhole_locks)];
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(lock, &node);
    entry = __pt_walk(va, &__level);
    mcs_unlock_isr_restore(lock, &node);

    *level = __level;
    return entry;
}

u64 __virt_to_phys(va_t va) 
{
    int level;
    void *entry = pt_walk(va, &level);
    
    u64 phys = 0;

    switch (level) {

        case PGTABLE_LEVEL_1GB:

            pdpte_huge_t *pdpte = entry;
            phys = (pdpte->fields.pfn << PAGE_SHIFT_1GB) + offsetof_1gb(va.val);
            break;

        case PGTABLE_LEVEL_2MB:

            pde_huge_t *pde = entry;
            phys = (pde->fields.pfn << PAGE_SHIFT_2MB) + va.fields.offset;
            break;

        case PGTABLE_LEVEL_4K:

            pte_t *pte = entry;
            phys = (pte->fields.pfn << PAGE_SHIFT_4KB) + offsetof_4k(va.val);
            break;

        case PGTABLE_LEVEL_NONE:
            break;
    }

    return phys;
}

u64 virt_to_phys(void *addr)
{
    /* low 2 mb of pd heap will be 4k pages, pd pfn and pd pfn io will require
       us to grab their corresponding locks as they are not static page tables
       unlike our heap page tables */

    va_t va = {.val = (u64)addr};

    if (va.fields.pml4 != PDPT_RESERVED_PML4_IDX)
        return __virt_to_phys(va);

    u64 phys = 0;
    switch (va.fields.pdpt) {

        case PD_PFN_PDPT_IDX:
        
            struct mcsnode pd_pfn_node = INITIALIZE_MCSNODE();

            mcs_lock_isr_save(&__pd_pfn_mapper.lock, &pd_pfn_node);
            phys = __virt_to_phys(va);
            mcs_unlock_isr_restore(&__pd_pfn_mapper.lock, &pd_pfn_node);

            break;

        case PD_PFN_IO_PDPT_IDX:

            struct mcsnode pd_pfn_io_node = INITIALIZE_MCSNODE();

            mcs_lock_isr_save(&__pd_pfn_io_mapper.lock, &pd_pfn_io_node);
            phys = __virt_to_phys(va);
            mcs_unlock_isr_restore(&__pd_pfn_io_mapper.lock, &pd_pfn_io_node);

            break;

        default:
            phys = __virt_to_phys(va);
            break;
    }

    return phys;
}

void init_mappers(void)
{
    mcslock_isr_init(&__pd_pfn_mapper.lock);
    mcslock_isr_init(&__pd_pfn_io_mapper.lock);

    bmp512_set_all(&__pd_pfn_mapper.bmp);
    bmp512_set_all(&__pd_pfn_io_mapper.bmp);

    for (u32 i = 0; i < ARRAY_LEN(keyhole_locks); i++)
        mcslock_isr_init(&keyhole_locks[i]);
}

void init_heap(void)
{
    u32 pfn_count = MAX_HEAP_SIZE / PAGE_SIZE;

    KBUG_ON(pfn_count > ARRAY_LEN(pd_heap));

    u32 allocated = 0;
    for (u32 i = 0; i < pfn_count; i++) {

        void *id;
        u64 phys = pma_alloc_pages(1, &id);
        
        if (phys == PMA_NULL)
            break;

        pd_heap[i].val = 0;
        pd_heap[i].fields.rw = 1;
        pd_heap[i].fields.present = 1;
        pd_heap[i].fields.ps = 1;
        pd_heap[i].fields.pfn = phys >> PAGE_SHIFT;

        allocated++;
    }

    struct twan_kernel *kernel = twan();

    kernel->mem.heap_start = (u64)PD_HEAP_START_ADDR;
    kernel->mem.heap_size = allocated * PAGE_SIZE;

    flush_tlb_local();
}

/* pfn mappers */

void *__map_pfn_noflush(u64 pfn)
{
    int idx = bmp512_ffs_unset(&__pd_pfn_mapper.bmp); 
    if (idx == -1)
        return NULL;

    pd_pfn[idx].val = 0;
    pd_pfn[idx].fields.rw = 1;
    pd_pfn[idx].fields.present = 1;
    pd_pfn[idx].fields.ps = 1;
    pd_pfn[idx].fields.pfn = pfn;

    u64 addr = (u64)PD_PFN_START_ADDR | (idx << PAGE_SHIFT_2MB);

    return (void *)addr;
}

void *__map_pfn_local(u64 pfn)
{
    void *addr = __map_pfn_io_noflush(pfn);
    if (addr)
        flush_tlb_page_local((u64)addr);

    return addr;
}

void __unmap_pfn_noflush(void *addr)
{
    int idx = pd_index_of((u64)addr);
    pd_pfn[idx].val = 0;
    bmp512_set(&__pd_pfn_mapper.bmp, idx);
}

void __unmap_pfn_local(void *addr)
{
    __unmap_pfn_noflush(addr);
    flush_tlb_page_local((u64)addr);
}

void __remap_pfn_noflush(void *addr, u32 pfn)
{
    pd_pfn[pd_index_of((u64)addr)].fields.pfn = pfn;
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
    u32 pfn = phys >> PAGE_SHIFT_2MB;
    u32 offset = phys % PAGE_SIZE_2MB;

    if (offset == 0)
        return __map_pfn_noflush(pfn);

    int idx = bmp512_ffs2_unset2(&__pd_pfn_mapper.bmp);
    if (idx == -1)
        return NULL;

    pd_pfn[idx].val = 0;
    pd_pfn[idx].fields.rw = 1;
    pd_pfn[idx].fields.present = 1;
    pd_pfn[idx].fields.ps = 1;
    pd_pfn[idx].fields.pfn = pfn;

    pd_pfn[idx + 1].val = 0;
    pd_pfn[idx + 1].fields.rw = 1;
    pd_pfn[idx + 1].fields.present = 1;
    pd_pfn[idx + 1].fields.ps = 1;
    pd_pfn[idx + 1].fields.pfn = pfn + 1;

    u64 addr = ((u64)PD_PFN_START_ADDR | (idx << PAGE_SHIFT_2MB) | offset);

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
    u8 *base = (void *)p2align_down((u64)addr, PAGE_SIZE_2MB);
    __unmap_pfn_noflush(base);

    u64 offset = (u64)addr % PAGE_SIZE_2MB;
    if (offset != 0) {
        u8 *stretch = base + PAGE_SIZE_2MB;
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

    pd_pfn_io[idx].val = 0;
    pd_pfn_io[idx].fields.rw = 1;
    pd_pfn_io[idx].fields.present = 1;
    pd_pfn_io[idx].fields.pwt = 1;
    pd_pfn_io[idx].fields.pcd = 1;
    pd_pfn_io[idx].fields.ps = 1;
    pd_pfn_io[idx].fields.pfn = pfn;

    u64 addr = ((u64)PD_PFN_IO_START_ADDR | (idx << PAGE_SHIFT_2MB));
    return (void *)addr;
}

void *__map_pfn_io_local(u64 pfn)
{
    void *addr = __map_pfn_io_noflush(pfn);
    if (addr)
        flush_tlb_page_local((u64)pfn);

    return addr;
}

void __unmap_pfn_io_noflush(void *addr)
{
    int idx = pd_index_of((u64)addr);
    pd_pfn_io[idx].val = 0;
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
    u32 pfn = phys >> PAGE_SHIFT_2MB;
    u32 offset = phys % PAGE_SIZE_2MB;

    if (offset == 0)
        return __map_pfn_io_noflush(pfn);

    int idx = bmp512_ffs2_unset2(&__pd_pfn_io_mapper.bmp);
    if (idx == -1)
        return NULL;

    pd_pfn_io[idx].val = 0;
    pd_pfn_io[idx].fields.rw = 1;
    pd_pfn_io[idx].fields.present = 1;
    pd_pfn_io[idx].fields.pwt = 1;
    pd_pfn_io[idx].fields.pcd = 1;
    pd_pfn_io[idx].fields.ps = 1;
    pd_pfn_io[idx].fields.pfn = pfn;

    pd_pfn_io[idx + 1].val = 0;
    pd_pfn_io[idx + 1].fields.rw = 1;
    pd_pfn_io[idx + 1].fields.present = 1;
    pd_pfn_io[idx + 1].fields.pwt = 1;
    pd_pfn_io[idx + 1].fields.pcd = 1;
    pd_pfn_io[idx + 1].fields.ps = 1;
    pd_pfn_io[idx + 1].fields.pfn = pfn + 1;

    u64 addr = ((u64)PD_PFN_IO_START_ADDR | (idx << PAGE_SHIFT_2MB) | offset);
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
    u8 *base = (void *)p2align_down((u64)addr, PAGE_SIZE_2MB);
    __unmap_pfn_io_noflush(base);

    u64 offset = (u64)addr % PAGE_SIZE_2MB;
    if (offset != 0) {
         u8 *stretch = base + PAGE_SIZE_2MB;
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

void __remap_keyhole_local(void *addr, u32 pfn)
{
    u32 idx = pd_index_of((u64)addr);
    pd_keyholes[idx].fields.rw = 1;
    pd_keyholes[idx].fields.present = 1;
    pd_keyholes[idx].fields.ps = 1;
    pd_keyholes[idx].fields.pfn = pfn;
    flush_tlb_page_local((u64)addr);
}