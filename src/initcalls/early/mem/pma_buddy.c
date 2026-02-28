#include <initcalls/early_initcalls_conf.h>
#if EARLY_MEM_PMA_BUDDY

#include <kernel/kapi.h>
#include <subsys/debug/kdbg/kdbg.h>
#include <subsys/mem/pma.h>
#include <lib/buddy_alloc.h>
#include <errno.h>

#define PMA_NUM_ARENAS 5
STATIC_ASSERT(PMA_NUM_ARENAS <= UINT32_MAX);

struct pma_buddy_arena
{
    struct buddy_arena arena;
    struct mcslock_isr lock;
};

static struct pma_buddy_arena arenas[PMA_NUM_ARENAS];

static atomic32_t num_arenas_initialized = INITIALIZE_ATOMIC32(0);
static struct mcslock_isr num_arenas_lock;

static int pma_buddy_init_range(u64 phys, size_t size)
{
    /* TODO: if the range is larger than the max for an arena, cut it up into
             multiple arenas */

    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&num_arenas_lock, &node);
    
    u32 num = atomic32_read(&num_arenas_initialized);

    if (num == ARRAY_LEN(arenas)) {
        mcs_unlock_isr_restore(&num_arenas_lock, &node);
        return -ENOSPC;
    }

    int ret = buddy_arena_init(&arenas[num].arena, phys, size);
    if (ret == 0)
        atomic32_inc(&num_arenas_initialized);

    mcs_unlock_isr_restore(&num_arenas_lock, &node);
    return ret;
}

static u64 pma_buddy_alloc_pages(u32 num_pages, void **id)
{
    u32 order = __buddy_get_order(num_pages);
    u32 num = atomic32_read(&num_arenas_initialized);

    for (u32 i = 0; i < num; i++) {

        struct pma_buddy_arena *arena = &arenas[i];

        struct mcsnode node = INITIALIZE_MCSNODE();

        mcs_lock_isr_save(&arena->lock, &node); 

        if (!__buddy_order_can_alloc(&arena->arena, order)) {
            mcs_unlock_isr_restore(&arena->lock, &node);
            continue;
        }
        
        u64 ret = __buddy_alloc(&arena->arena, order);

        if (KBUG_ON(ret == PMA_NULL)) {
            mcs_unlock_isr_restore(&arena->lock, &node);
            continue;
        }

        mcs_unlock_isr_restore(&arena->lock, &node);

        if (id)
            *id = (void *)(long)i;

        return ret;
    }

    return PMA_NULL;
}

static int pma_buddy_free_pages(void *id, u64 phys, u32 num_pages)
{
    long _id = (long)id;
    if (_id < 0 || _id >= atomic32_read(&num_arenas_initialized))
        return -EINVAL;

    u32 order = __buddy_get_order(num_pages);

    struct pma_buddy_arena *arena = &arenas[_id];

    struct mcsnode node = INITIALIZE_MCSNODE();
    mcs_lock_isr_save(&arena->lock, &node);

    __buddy_free(&arena->arena, phys, order);

    mcs_unlock_isr_restore(&arena->lock, &node);

    return 0;
}

static struct pma_interface pma_interface = {
    .pma_init_range_func = pma_buddy_init_range,
    .pma_alloc_pages_func = pma_buddy_alloc_pages,
    .pma_free_pages_func = pma_buddy_free_pages
};

static __early_initcall void pma_buddy_init(void)
{
    pma_init(&pma_interface);    
}

REGISTER_EARLY_INITCALL(pma_buddy_init, pma_buddy_init, 
                        EARLY_MEM_PMA_BUDDY_ORDER);

#endif