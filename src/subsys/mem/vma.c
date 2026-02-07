#include <include/subsys/mem/vma.h>
#include <include/subsys/debug/kdbg/kdbg.h>
#include <include/errno.h>

static struct vma vma_global;

int vma_init(struct vma_interface *interface)
{
    if (!interface)
        return -EINVAL;

    if (vma_global.initialized)
        return -EALREADY;

    vma_global.interface = interface;
    vma_global.initialized = true;

    return 0;
}

bool is_vma_initialized(void)
{
    return vma_global.initialized;
}