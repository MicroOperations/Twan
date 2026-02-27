#include <initcalls/late_initcalls_conf.h>
#if LATE_MEM_RTALLOC_TLSF

#include <kernel/kapi.h>
#include <kernel/mem/mmu/tlb.h>
#include <lib/tlsf_alloc.h>
#include <subsys/mem/rtalloc.h>
#include <subsys/mem/vma.h>
#include <subsys/mem/pma.h>
#include <generated/autoconf.h>

/* TODO: alter tlsf library and this to use more fine grained locks, global lock
   on the allocator wouldn't scale very well */

#define RTALLOC_TLSF_NUM_PARTITIONS \
    (CONFIG_KERNEL_MAX_HEAP_SIZE / VMA_PARTITION_SIZE)

#define RTALLOC_TLSF_NUM_PAGES (CONFIG_KERNEL_MAX_HEAP_SIZE / PAGE_SIZE)

static struct tlsf tlsf_global;

static struct vma_descriptor tlsf_descriptor;
static DEFINE_VMA_PARTITION_TABLE(tlsf_table, RTALLOC_TLSF_NUM_PARTITIONS);
REGISTER_VMA_PARTITION_TABLE(tlsf_table, virt_to_phys_static(tlsf_table),
                             VMA_PARTITION_TABLE_COUNT(tlsf_table),
                             &tlsf_descriptor);

static void *rtalloc_tlsf(size_t size)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    tlsf_lock(&tlsf_global, &node);
    void *addr = __tlsf_alloc(&tlsf_global, size);
    tlsf_unlock(&tlsf_global, &node);

    return addr;
}

static void rtfree_tlsf(void *addr)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    tlsf_lock(&tlsf_global, &node);
    __tlsf_free(&tlsf_global, addr);
    tlsf_unlock(&tlsf_global, &node);
}

static void *rtrealloc_tlsf(void *addr, size_t size)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    tlsf_lock(&tlsf_global, &node);
    void *new_addr = __tlsf_realloc(&tlsf_global, addr, size);
    tlsf_unlock(&tlsf_global, &node);
    
    return new_addr;
}

static void *rtalloc_p2aligned_tlsf(size_t size, u32 alignment)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    tlsf_lock(&tlsf_global, &node);
    void *addr = __tlsf_alloc_p2aligned(&tlsf_global, size, alignment);
    tlsf_unlock(&tlsf_global, &node);

    return addr;
}

static void rtfree_p2aligned_tlsf(void *addr)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    tlsf_lock(&tlsf_global, &node);
    __tlsf_free_p2aligned(&tlsf_global, addr);
    tlsf_unlock(&tlsf_global, &node);
}

static void *rtrealloc_p2aligned_tlsf(void *addr, size_t size, u32 alignment)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    tlsf_lock(&tlsf_global, &node);

    void *new_addr =  __tlsf_realloc_p2aligned(&tlsf_global, addr, size,
                                               alignment);
                                               
    tlsf_unlock(&tlsf_global, &node);

    return new_addr;
}

static struct rtalloc_interface rtalloc_interface = {
    .rtalloc_func = rtalloc_tlsf,
    .rtfree_func = rtfree_tlsf,
    .rtrealloc_func = rtrealloc_tlsf,

    .rtalloc_p2aligned_func = rtalloc_p2aligned_tlsf,
    .rtfree_p2aligned_func = rtfree_p2aligned_tlsf,
    .rtrealloc_p2aligned_func = rtrealloc_p2aligned_tlsf
};

static __late_initcall void rtalloc_tlsf_init(void)
{
    if (RTALLOC_TLSF_NUM_PAGES == 0)
        return;

    u32 count = tlsf_descriptor.num_partitions_valid;
    if (count == 0 )
        return;
    
    u64 heap_start = tlsf_descriptor.addr;
    size_t heap_size = tlsf_descriptor.size;

    u32 num_pages = 0;
    for (u32 i = 0; i < count; i++) {

        struct vma_partition *partition = 
            vma_partition_table_get(tlsf_table, i);

        for (u32 i = 0; i < VMA_PARTITION_ENTRY_COUNT; i++) {

            if (num_pages == RTALLOC_TLSF_NUM_PAGES)
                goto done;

            void *id;
            u64 pa = pma_alloc_pages(1, &id);

            vma_partition_remap(partition, num_pages, vma_pa_to_pfn(pa));
            num_pages++;
        }
    }

    flush_tlb_global(true);

done:
    
    if (__tlsf_init(&tlsf_global, heap_start, heap_size) == 0)
        rtalloc_init(&rtalloc_interface);
}

REGISTER_LATE_INITCALL(rtalloc_tlsf, rtalloc_tlsf_init, 
                       LATE_MEM_RTALLOC_TLSF_ORDER);
#endif