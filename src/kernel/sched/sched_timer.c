#include <kernel/sched/sched_timer.h>
#include <kernel/isr/base_isrs.h>
#include <kernel/apic/apic.h>
#include <lib/libtwanvisor/libvcalls.h>
#include <lib/libtwanvisor/libvc.h>
#include <subsys/debug/kdbg/kdbg.h>

void sched_timer_init(u32 time_slice_ms)
{
#if CONFIG_SUBSYS_TWANVISOR

    if (twan()->flags.fields.twanvisor_on != 0) {

        u32 ticks = ms_to_ticks(time_slice_ms, 
                                this_cpu_data()->vtimer_frequency_hz);

        this_cpu_data()->sched_ticks = ticks;

        sched_timer_enable();
        return;
    }

#endif

    this_cpu_data()->sched_ticks = 
        lapic_timer_init(SCHED_TIMER_VECTOR, time_slice_ms);
}

void sched_timer_disable(void)
{
#if CONFIG_SUBSYS_TWANVISOR

    if (twan()->flags.fields.twanvisor_on != 0) {
        tv_vdisarm_timern(PV_SCHED_TIMER);
        return;
    }

#endif

    lapic_timer_disable();
}

void sched_timer_enable(void)
{
    u32 ticks = this_cpu_data()->sched_ticks;

#if CONFIG_SUBSYS_TWANVISOR

    if (twan()->flags.fields.twanvisor_on != 0) {

        KBUG_ON(tv_varm_timern(SCHED_TIMER_VECTOR, PV_SCHED_TIMER, 
                ticks, true, false) < 0);

        return;
    }

#endif

    lapic_timer_enable(ticks);
}

bool sched_is_timer_pending(void)
{
#if CONFIG_SUBSYS_TWANVISOR

    if (twan()->flags.fields.twanvisor_on != 0)
        return tv_vintc_is_pending(SCHED_TIMER_VECTOR);

#endif

    return is_lapic_timer_pending(SCHED_TIMER_VECTOR);
}