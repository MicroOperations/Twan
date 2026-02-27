#include <subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <subsys/twanvisor/vsched/vsched.h>
#include <subsys/twanvisor/twanvisor.h>
#include <subsys/twanvisor/vsched/vsched_yield.h>
#include <subsys/twanvisor/vsched/vsched_timer.h>
#include <subsys/twanvisor/vportal/vrecovery.h>

void __vsched_put(struct vcpu *vcpu, bool put_ctx, 
                 struct interrupt_info *ctx)
{
    vcpu->vsched_metadata.state = VREADY;
    vcpu->vsched_metadata.rearm = false;

    vcpu->flags.fields.preempted = 1;

    if (put_ctx)
        vsched_put_ctx(vcpu, ctx);

    __vsched_push(vcpu);
}

void __vsched_put_paused(struct vcpu *vcpu, bool pv_spin, bool put_ctx,
                         struct interrupt_info *ctx)
{
    vcpu->vsched_metadata.state = pv_spin ? VPV_SPINNING : VPAUSED;
    vcpu->vsched_metadata.rearm = false;

    vcpu->flags.fields.preempted = 1;

    if (put_ctx)
        vsched_put_ctx(vcpu, ctx);

    __vsched_push_paused(vcpu);
}

void vsched_put(struct vcpu *vcpu, bool put_ctx, 
                struct interrupt_info *ctx)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vsched->lock, &node);
    
    if (vcpu->vsched_metadata.terminate) {
        
        vmcs_unlock_isr_restore(&vsched->lock, &node);
        vdo_transitioning_recover(vcpu);
        return;
    }

    __vsched_put(vcpu, put_ctx, ctx);

    vmcs_unlock_isr_restore(&vsched->lock, &node);
}

bool vsched_put_paused(struct vcpu *vcpu, bool pv_spin, bool put_ctx, 
                       struct interrupt_info *ctx)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vsched->lock, &node);
    
    if (vcpu->vsched_metadata.terminate) {
        
        vmcs_unlock_isr_restore(&vsched->lock, &node);
        vdo_transitioning_recover(vcpu);
        return true;
    }

    bool ret = false;

    if (pv_spin) {

        vcpu_pv_spin_state_t state = vcpu->vsched_metadata.pv_spin_state;

        switch (state) {

            case VSPIN_NONE:
                __vsched_put_paused(vcpu, true, put_ctx, ctx);
                vcpu->vsched_metadata.pv_spin_state = VSPIN_PAUSED;
                ret = true;
                break;

            case VSPIN_KICKED:
                vcpu->vsched_metadata.pv_spin_state = VSPIN_NONE;
                break;

            default:
                VBUG_ON(true);
                break;
        }

    } else {

        __vsched_put_paused(vcpu, false, put_ctx, ctx);
        ret = true;
    }

    vmcs_unlock_isr_restore(&vsched->lock, &node);

    return ret;
}

struct vcpu *__vsched_get(struct interrupt_info *ctx)
{
    struct vcpu *vcpu = __vsched_pop();
    if (vcpu) {

        vthis_cpu_data()->current_vcpu = vcpu;

        vcpu->vsched_metadata.state = VTRANSITIONING;

        vsched_set_ctx(vcpu, ctx);

        vcpu->vsched_metadata.current_time_slice_ticks = 
            vcpu->vsched_metadata.time_slice_ticks;
    }

    return vcpu;
}

struct vcpu *vsched_get(struct interrupt_info *ctx)
{
    struct vscheduler *vsched = vthis_vscheduler();

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vsched->lock, &node);

    struct vcpu *vcpu = __vsched_get(ctx);    

    u32 ticks = vcpu->vsched_metadata.current_time_slice_ticks;

    vmcs_unlock_isr_restore(&vsched->lock, &node);

    vsched_arm_timer(ticks);
    return vcpu;
}

void vthis_vsched_reschedule(struct vcpu *current, 
                             struct interrupt_info *ctx)
{
    struct vscheduler *vsched = vthis_vscheduler();

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vsched->lock, &node);

    if (current->vsched_metadata.terminate) {

        vmcs_unlock_isr_restore(&vsched->lock, &node);

        vdo_transitioning_recover(current);
        vsched_get_spin(ctx);
        return;
    }
    
    if (__vsched_is_current_preemptible(current)) {
        __vsched_put(current, true, ctx);
        __vsched_get(ctx);
    } else {

        current->vsched_metadata.current_time_slice_ticks = 
            current->vsched_metadata.time_slice_ticks;
    }

    u32 ticks = vcurrent_vcpu()->vsched_metadata.current_time_slice_ticks;
    vmcs_unlock_isr_restore(&vsched->lock, &node);

    vsched_arm_timer(ticks);
}

void vsched_preempt_isr(void)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    struct interrupt_info *ctx = vthis_cpu->vcpu_ctx;

    struct vcpu *current = vcurrent_vcpu();
    if (!current)
        return;

    if (current->preemption_count > 0) {

        if (vsched_should_request_yield(current)) {
            current->flags.fields.yield_request = 1;
            return;
        }

        struct vscheduler *vsched = vthis_vscheduler();
        struct mcsnode vsched_node = INITIALIZE_MCSNODE();

        vmcs_lock_isr_save(&vsched->lock, &vsched_node);
        u32 ticks = current->vsched_metadata.current_time_slice_ticks;
        vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);

        vsched_arm_timer(ticks);
        return;
    }

    current->flags.fields.yield_request = 0;
    vthis_vsched_reschedule(current, ctx);
}

#endif