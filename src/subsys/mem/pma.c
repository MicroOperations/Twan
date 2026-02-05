#include <include/subsys/mem/pma.h>
#include <include/subsys/debug/kdbg/kdbg.h>
#include <include/errno.h>

static struct pma pma_global;

int pma_init(struct pma_interface *interface)
{
    if (!interface)
        return -EINVAL;

    if (pma_global.initialized)
        return -EALREADY;

    pma_global.interface = interface;
    pma_global.initialized = true;

    return 0;
}

bool is_pma_initialized(void)
{
    return pma_global.initialized;
}

int pma_init_range(u64 phys, size_t size)
{
    if (!pma_global.initialized)
        return -EINVAL;

    KBUG_ON(!pma_global.interface);
    KBUG_ON(!pma_global.interface->pma_init_range_func);

    return INDIRECT_BRANCH_SAFE(
        pma_global.interface->pma_init_range_func(phys, size));
}


u64 pma_alloc_pages(u32 num_pages, void **id)
{
    if (!pma_global.initialized)
        return 0;
        
    KBUG_ON(!pma_global.interface);
    KBUG_ON(!pma_global.interface->pma_alloc_pages_func);

    return INDIRECT_BRANCH_SAFE(
        pma_global.interface->pma_alloc_pages_func(num_pages, id));
}

int pma_free_pages(void *id, u64 phys, u32 num_pages)
{
    if (!pma_global.initialized)
        return -EINVAL;
        
    KBUG_ON(!pma_global.interface);
    KBUG_ON(!pma_global.interface->pma_free_pages_func);

    return INDIRECT_BRANCH_SAFE(
        pma_global.interface->pma_free_pages_func(id, phys, num_pages));
}