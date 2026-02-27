#include <subsys/mem/rtalloc.h>
#include <subsys/debug/kdbg/kdbg.h>
#include <errno.h>

static struct rtalloc rtalloc_global;

int rtalloc_init(struct rtalloc_interface *interface)
{
    if (!interface)
        return -EINVAL;

    if (rtalloc_global.initialized)
        return -EALREADY;

    rtalloc_global.interface = interface;
    rtalloc_global.initialized = true;

    return 0;
}

bool is_rtalloc_initialized(void)
{
    return rtalloc_global.initialized;
}

void *rtalloc(size_t size)
{
    if (!rtalloc_global.initialized)
        return NULL;

    KBUG_ON(!rtalloc_global.interface);
    KBUG_ON(!rtalloc_global.interface->rtalloc_func);

    return INDIRECT_BRANCH_SAFE(rtalloc_global.interface->rtalloc_func(size));
}

void rtfree(void *addr)
{
    if (rtalloc_global.initialized) {

        KBUG_ON(!rtalloc_global.interface);
        KBUG_ON(!rtalloc_global.interface->rtfree_func);

        INDIRECT_BRANCH_SAFE(rtalloc_global.interface->rtfree_func(addr));
    }
}

void *rtrealloc(void *addr, size_t size)
{
    if (!rtalloc_global.initialized)
        return NULL;

    KBUG_ON(!rtalloc_global.interface);
    KBUG_ON(!rtalloc_global.interface->rtrealloc_func);

    return INDIRECT_BRANCH_SAFE(
            rtalloc_global.interface->rtrealloc_func(addr, size));
}

void *rtallocz(size_t size)
{
    void *addr = rtalloc(size);
    if (addr)
        memset(addr, 0, size);

    return addr;
}

void *rtalloc_p2aligned(size_t size, u32 alignment)
{
    if (!rtalloc_global.initialized)
        return NULL;

    KBUG_ON(!rtalloc_global.interface);
    KBUG_ON(!rtalloc_global.interface->rtalloc_p2aligned_func);

    return INDIRECT_BRANCH_SAFE(
            rtalloc_global.interface->rtalloc_p2aligned_func(size, alignment));
}

void rtfree_p2aligned(void *addr)
{
    if (rtalloc_global.initialized) {

        KBUG_ON(!rtalloc_global.interface);
        KBUG_ON(!rtalloc_global.interface->rtfree_func);

        INDIRECT_BRANCH_SAFE(
            rtalloc_global.interface->rtfree_p2aligned_func(addr));
    }
}

void *rtrealloc_p2aligned(void *addr, size_t size, u32 alignment)
{
    if (!rtalloc_global.initialized)
        return NULL;

    KBUG_ON(!rtalloc_global.interface);
    KBUG_ON(!rtalloc_global.interface->rtrealloc_func);

    return INDIRECT_BRANCH_SAFE(
            rtalloc_global.interface->rtrealloc_p2aligned_func(
                addr, size, alignment));
}

void *rtallocz_p2aligned(size_t size, u32 alignment)
{
    void *addr = rtalloc_p2aligned(size, alignment);
    if (addr)
        memset(addr, 0, size);
    
    return addr;
}