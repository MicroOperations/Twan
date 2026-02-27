#include <subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <subsys/twanvisor/vsched/vsched_timer.h>
#include <subsys/twanvisor/twanvisor.h>
#include <subsys/twanvisor/visr/visr_index.h>
#include <subsys/twanvisor/vemulate/vemulate_utils.h>

bool vis_sched_timer_done(void)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    u64 flags = read_flags_and_disable_interrupts();

    bool ret = vthis_cpu->sec_flags.fields.sched_timer_armed != 0 &&
               vis_lapic_oneshot_done();
    
    if (ret)
        vthis_cpu->sec_flags.fields.sched_timer_armed = 0;

    write_flags(flags);
    return ret;
}

void vsched_timer_reload(u32 ticks)
{
    vset_lapic_oneshot(VSCHED_TIMER_VECTOR, ticks, VSCHED_LAPIC_DCR);
}

void vsched_arm_timer(u32 ticks)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    u64 flags = read_flags_and_disable_interrupts();

    if (ticks != 0) {
        vthis_cpu->sec_flags.fields.sched_timer_armed = 1;
        vsched_timer_reload(ticks);
    } else {
        vthis_cpu->sec_flags.fields.sched_timer_armed = 0;
    }

    write_flags(flags);
}

#endif