#include <subsys/watchdog/watchdog.h>
#if CONFIG_SUBSYS_WATCHDOG

#include <subsys/debug/kdbg/kdyn_assert.h>
#include <errno.h>

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

        KDYNAMIC_ASSERT(watchdog_global.interface);
        KDYNAMIC_ASSERT(watchdog_global.interface->watchdog_pet_func);

        watchdog_global.interface->watchdog_pet_func();
    }
}

int watchdog_arm(u32 ticks)
{
    if (!is_watchdog_initialized())
        return 0;

    KDYNAMIC_ASSERT(watchdog_global.interface);
    KDYNAMIC_ASSERT(watchdog_global.interface->watchdog_arm_func);

    return watchdog_global.interface->watchdog_arm_func(ticks);
}

int watchdog_disarm(void)
{
    if (!is_watchdog_initialized())
        return 0;

    KDYNAMIC_ASSERT(watchdog_global.interface);
    KDYNAMIC_ASSERT(watchdog_global.interface->watchdog_disarm_func);

    return watchdog_global.interface->watchdog_disarm_func();
}

#endif