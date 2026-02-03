#include <include/subsys/watchdog/watchdog.h>
#if CONFIG_SUBSYS_WATCHDOG

#include <include/subsys/debug/kdbg/kdbg.h>
#include <include/errno.h>

static struct watchdog watchdog_global;

int watchdog_init(struct watchdog_interface *interface)
{
    if (!interface)
        return -EINVAL;

    if (watchdog_global.initialized)
        return -EALREADY;

    watchdog_global.interface = interface;
    watchdog_global.initialized = true;

    return 0;
}

bool is_watchdog_initialized(void)
{
    return watchdog_global.initialized;
}

void watchdog_pet(void)
{
    if (is_watchdog_initialized()) {

        KBUG_ON(!watchdog_global.interface);
        KBUG_ON(!watchdog_global.interface->watchdog_pet_func);

        watchdog_global.interface->watchdog_pet_func();
    }
}

int watchdog_arm(u32 ticks)
{
    if (!is_watchdog_initialized())
        return 0;

    KBUG_ON(!watchdog_global.interface);
    KBUG_ON(!watchdog_global.interface->watchdog_arm_func);

    return watchdog_global.interface->watchdog_arm_func(ticks);
}

int watchdog_disarm(void)
{
    if (!is_watchdog_initialized())
        return 0;

    KBUG_ON(!watchdog_global.interface);
    KBUG_ON(!watchdog_global.interface->watchdog_disarm_func);

    return watchdog_global.interface->watchdog_disarm_func();
}

#endif