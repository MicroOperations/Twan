#include <subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <subsys/twanvisor/vsched/vsched_yield.h>
#include <subsys/twanvisor/twanvisor.h>
#include <subsys/twanvisor/vsched/vsched.h>
#include <subsys/twanvisor/vsched/vsched_timer.h>

bool __vsched_is_current_preemptible(struct vcpu *current)
{
    u8 criticality_level = __sched_mcs_read_criticality_level();
    u8 criticality = current->vsched_metadata.criticality;

    u8 next_criticality;
    return __vsched_get_bucket(&next_criticality) && 
                (criticality < criticality_level || 
                 next_criticality >= criticality_level);
}

bool vsched_should_request_yield(struct vcpu *current)
{
    struct vscheduler *vsched = vthis_vscheduler();

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vsched->lock, &node);

    bool ret = __vsched_is_current_preemptible(current);

    vmcs_unlock_isr_restore(&vsched->lock, &node);
    return ret;
}

void vsched_yield_ipi(__unused u64 unused1)
{
    struct vcpu *current = vcurrent_vcpu();
    struct interrupt_info *ctx = vthis_cpu_data()->vcpu_ctx;
    
    current->flags.fields.yield_request = 0;
    vthis_vsched_reschedule(current, ctx);
}

void vsched_yield(void)
{
    vemulate_self_ipi(vsched_yield_ipi, 0);
}

void vsched_pause_ipi(u64 pv_spin)
{
    struct vcpu *current = vcurrent_vcpu();
    struct interrupt_info *ctx = vthis_cpu_data()->vcpu_ctx;

    bool yield_requested = current->flags.fields.yield_request != 0;
    current->flags.fields.yield_request = 0;

    if (vsched_put_paused(current, pv_spin != 0, true, ctx))
        vsched_get_spin(ctx);
    else if (yield_requested)
        current->flags.fields.yield_request = 1;
}

void vsched_pause(bool pv_spin)
{
    vemulate_self_ipi(vsched_pause_ipi, pv_spin);
}

void vsched_recover_ipi(__unused u64 unused1)
{
    struct interrupt_info *ctx = vthis_cpu_data()->vcpu_ctx;
    vsched_get_spin(ctx);
}

void vsched_recover(void)
{
    vemulate_self_ipi(vsched_recover_ipi, 0);
}

void vsched_idle_yield_ipi(__unused u64 unused0)
{
    struct vcpu *current = vcurrent_vcpu();
    struct interrupt_info *ctx = vthis_cpu_data()->vcpu_ctx;

    vsched_put_ctx(current, ctx);
    vsched_get_spin(ctx);
}

void vsched_idle_yield(void)
{
    vemulate_self_ipi(vsched_idle_yield_ipi, 0);
}

#endif