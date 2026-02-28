#include <subsys/time/clock.h>
#if CONFIG_SUBSYS_CLOCK

#include <subsys/debug/kdbg/kdyn_assert.h>
#include <errno.h>

static struct clock clock_global;

int clock_init(struct clock_interface *interface)
{
    if (!interface)
        return -EINVAL;

    if (clock_global.initialized)
        return -EALREADY;

    clock_global.interface = interface;
    clock_global.initialized = true;
    
    return 0;
}

bool is_clock_initialized(void)
{
    return clock_global.initialized;   
}

void clock_read(struct clock_time *clock_time)
{
    if (clock_global.initialized) {

        KDYNAMIC_ASSERT(clock_global.interface);
        KDYNAMIC_ASSERT(clock_global.interface->clock_read_func);

        INDIRECT_BRANCH_SAFE(
            clock_global.interface->clock_read_func(clock_time));
    }
}

void clock_callback_disable_alarm(void)
{
    if (clock_global.initialized) {

        KDYNAMIC_ASSERT(clock_global.interface);
        KDYNAMIC_ASSERT(clock_global.interface->clock_callback_disable_alarm_func);

        INDIRECT_BRANCH_SAFE(
            clock_global.interface->clock_callback_disable_alarm_func());
    }
}

void clock_callback_set_alarm(clock_alarm_callback_func_t callback,
                              u8 seconds, u8 minutes, u8 hours)
{
    if (clock_global.initialized) {

        KDYNAMIC_ASSERT(clock_global.interface);
        KDYNAMIC_ASSERT(clock_global.interface->clock_callback_set_alarm_func);

        INDIRECT_BRANCH_SAFE(
            clock_global.interface->clock_callback_set_alarm_func(
                callback, seconds, minutes, hours));
    }
}

void clock_disable_alarm(void)
{
    if (clock_global.initialized) {

        KDYNAMIC_ASSERT(clock_global.interface);
        KDYNAMIC_ASSERT(clock_global.interface->clock_disable_alarm_func);

        INDIRECT_BRANCH_SAFE(
            clock_global.interface->clock_disable_alarm_func());
    }
}

void clock_set_alarm(clock_alarm_callback_func_t callback, 
                     u8 seconds, u8 minutes, u8 hours)
{
    if (clock_global.initialized) {

        KDYNAMIC_ASSERT(clock_global.interface);
        KDYNAMIC_ASSERT(clock_global.interface->clock_set_alarm_func);

        INDIRECT_BRANCH_SAFE(
            clock_global.interface->clock_set_alarm_func(
                callback, seconds, minutes, hours));
    }
}

#endif