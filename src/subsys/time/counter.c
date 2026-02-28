#include <subsys/time/counter.h>
#include <subsys/debug/kdbg/kdyn_assert.h>
#include <errno.h>

static struct counter counter_global;

int counter_init(struct counter_interface *interface)
{   
    if (!interface)
        return -EINVAL;

    if (counter_global.initialized)
        return -EALREADY;

    counter_global.interface = interface;
    counter_global.initialized = true;
    
    return 0;
}

bool is_counter_initialized(void)
{
    return counter_global.initialized;
}

u64 read_counter(void)
{
    if (!counter_global.initialized)
        return 0;

    KDYNAMIC_ASSERT(counter_global.interface);
    KDYNAMIC_ASSERT(counter_global.interface->read_counter_func);

    return INDIRECT_BRANCH_SAFE(counter_global.interface->read_counter_func());
}

u64 counter_period_fs(void)
{
    if (!counter_global.initialized)
        return 0;

    KDYNAMIC_ASSERT(counter_global.interface);
    KDYNAMIC_ASSERT(counter_global.interface->counter_period_fs_func);
    
    return INDIRECT_BRANCH_SAFE(
            counter_global.interface->counter_period_fs_func());
}

u64 counter_frequency_hz(void)
{
    if (!counter_global.initialized)
        return 0;

    KDYNAMIC_ASSERT(counter_global.interface);
    KDYNAMIC_ASSERT(counter_global.interface->counter_frequency_hz_func);

    return INDIRECT_BRANCH_SAFE(
            counter_global.interface->counter_frequency_hz_func());
}