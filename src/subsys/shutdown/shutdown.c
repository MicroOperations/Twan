#include <subsys/shutdown/shutdown.h>
#if CONFIG_SUBSYS_SHUTDOWN

#include <subsys/debug/kdbg/kdbg.h>
#include <errno.h>

static struct shutdown shutdown_global;

int shutdown_init(struct shutdown_interface *interface)
{
    if (!interface)
        return -EINVAL;

    if (shutdown_global.initialized)
        return -EALREADY;

    shutdown_global.interface = interface;
    shutdown_global.initialized = true;

    return 0;
}

bool is_shutdown_initialized(void)
{
    return shutdown_global.initialized;
}

void shutdown_system(void)
{
    if (shutdown_global.initialized) {

        KBUG_ON(!shutdown_global.interface);
        KBUG_ON(!shutdown_global.interface->shutdown_system_func);

        INDIRECT_BRANCH_SAFE(shutdown_global.interface->shutdown_system_func());
    }
}

#endif