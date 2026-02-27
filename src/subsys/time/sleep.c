#include <subsys/time/sleep.h>
#if CONFIG_SUBSYS_SLEEP

#include <kernel/kernel.h>
#include <subsys/debug/kdbg/kdbg.h>
#include <errno.h>

static struct sleep sleep_global;

int sleep_init(struct sleep_interface *interface)
{
    if (!interface)
        return -EINVAL;

    if (sleep_global.initialized)
        return -EALREADY;

    sleep_global.interface = interface;
    sleep_global.initialized = true;

    return 0;
}

bool is_sleep_initialized(void)
{
    return sleep_global.initialized;
}

void sleep_ticks(u32 ticks)
{
    KBUG_ON(this_cpu_data()->handling_isr);

    if (sleep_global.initialized) {

        KBUG_ON(!sleep_global.interface);
        KBUG_ON(!sleep_global.interface->sleep_ticks_func);
            
        INDIRECT_BRANCH_SAFE(sleep_global.interface->sleep_ticks_func(ticks));
    }
}

u64 sleep_period_fs(void)
{
    if (!sleep_global.initialized)
        return 0;
    
    KBUG_ON(!sleep_global.interface);
    KBUG_ON(!sleep_global.interface->sleep_period_fs_func);

    return INDIRECT_BRANCH_SAFE(sleep_global.interface->sleep_period_fs_func());
}

u64 sleep_frequency_hz(void)
{
    if (!sleep_global.initialized)
        return 0;

    KBUG_ON(!sleep_global.interface);
    KBUG_ON(!sleep_global.interface->sleep_frequency_hz_func);

    return INDIRECT_BRANCH_SAFE(
            sleep_global.interface->sleep_frequency_hz_func());
}

#endif