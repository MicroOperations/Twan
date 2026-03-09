#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vportal/vcalls.h>
#include <subsys/twanvisor/vemulate/vemulate_utils.h>
#include <subsys/twanvisor/vsched/vsched_yield.h>
#include <subsys/twanvisor/vportal/vrecovery.h>
#include <subsys/twanvisor/vportal/vcreate.h>
#include <subsys/twanvisor/vdbg/vdyn_assert.h>

/* long VSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR(u8 vector) */
static long vsubscribe_external_interrupt_vector(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    long ret = 0;

    u8 vector = vcall_arg1_8(vregs);

    struct vcpu *current = vcurrent_vcpu();
    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vthis_cpu->dispatch_lock, &node);

    if (bmp256_test(&current->visr_metadata.allowed_external_vectors, vector)) {
        
        struct dq *dq = &vthis_cpu->dispatch_vectors[vector];
        struct list_double *dispatch_node = &current->vdispatch_nodes[vector];
        bool pend = vemu_is_interrupt_pending(current, vector, vector == NMI);

        bmp256_set(&current->visr_metadata.subscribed_vectors, vector);

        if (!pend && !dq_is_queued(dq, dispatch_node))
            dq_pushback(dq, dispatch_node);
        
    } else {
        ret = -EPERM;
    }

    vmcs_unlock_isr_restore(&vthis_cpu->dispatch_lock, &node);
    return ret;
}

/* long VUNSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR(u8 vector) */
static long vunsubscribe_external_interrupt_vector(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    long ret = 0;

    u8 vector = vcall_arg1_8(vregs);

    struct vcpu *current = vcurrent_vcpu();
    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vthis_cpu->dispatch_lock, &node);

    if (bmp256_test(&current->visr_metadata.allowed_external_vectors, vector)) {
        
        struct dq *dq = &vthis_cpu->dispatch_vectors[vector];
        struct list_double *dispatch_node = &current->vdispatch_nodes[vector];

        bmp256_unset(&current->visr_metadata.subscribed_vectors, vector);

        if (dq_is_queued(dq, dispatch_node))
            dq_dequeue(dq, dispatch_node);
        
    } else {
        ret = -EPERM;
    }

    vmcs_unlock_isr_restore(&vthis_cpu->dispatch_lock, &node);
    return ret;   
}

/* long VIPI(u32 processor_id, u32 delivery_mode, u8 vector, bool nmi) */
static long vipi(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    long ret = 0;

    u32 target_processor_id = vcall_arg1_32(vregs);
    u32 delivery_mode = vcall_arg2_32(vregs);
    u8 vector = vcall_arg3_8(vregs);
    bool nmi = vcall_arg4_32(vregs);

    switch (delivery_mode) {

        case VIPI_DM_EXTERNAL:

            ret = vemu_inject_external_interrupt_local(target_processor_id,
                                                       vector, nmi);
            break;

#if !CONFIG_TWANVISOR_VSCHED_STRICT

        case VIPI_DM_UNPAUSE:

            if (vcurrent_vcpu()->processor_id == target_processor_id) {
                ret = -EINVAL;
                break;
            }

            ret = vemu_unpause_vcpu_local(target_processor_id, true, vector, 
                                          nmi);
            break;

#endif

        default:
            ret = -EINVAL;
            break;
    } 

    return ret;
}

/* long VIPI_FAR(u8 vid, u32 processor_id, u32 delivery_mode, u8 vector, bool nmi) */
static long vipi_far(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    long ret = 0;

    u8 vid = vcall_arg1_8(vregs);
    u32 processor_id = vcall_arg2_32(vregs);
    u32 delivery_mode = vcall_arg3_32(vregs);
    u8 vector = vcall_arg4_8(vregs);
    bool nmi = vcall_arg5_32(vregs);

    switch (delivery_mode) {

        case VIPI_DM_EXTERNAL:

            ret = vemu_inject_external_interrupt_far(vid, processor_id, vector, 
                                                     nmi);
            break;

#if !CONFIG_TWANVISOR_VSCHED_STRICT

        case VIPI_DM_UNPAUSE:

            ret = vemu_unpause_vcpu_far(vid, processor_id, true, vector, nmi);
            break;
        
#endif

        default:
            ret = -EINVAL;
            break;
    }

    return ret;
}

/* long VTLB_SHOOTDOWN(u8 target_vid) */
static long vtlb_shootdown(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vid = vcall_arg1_8(vregs);
    return vemu_tlb_invalidate(vid);
}

/* long VARM_TIMERN(u8 vector, u8 timer_n, u32 ticks, bool periodic, 
                    bool nmi) */
static long varm_timern(struct vregs *vregs)
{
    u8 vector = vcall_arg1_8(vregs);
    u8 timer_n = vcall_arg2_8(vregs);
    u32 ticks = vcall_arg3_32(vregs);
    bool periodic = vcall_arg4_32(vregs);
    bool nmi = vcall_arg5_32(vregs);
    
    if (timer_n >= VNUM_VTIMERS || (nmi && vector != NMI)) {

        vcurrent_vcpu_enable_preemption();
        return -EINVAL;
    }

    struct vcpu *current = vcurrent_vcpu();
    struct vtimer *vtimer = &current->timers[timer_n];

    if (vtimer->timer.fields.armed) {

        vcurrent_vcpu_enable_preemption();
        return -EALREADY;
    }

    struct delta_chain *chain = &current->timer_chain;

    if (delta_chain_is_empty(chain)) {

        __vmwrite(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, ticks);

        vmx_pinbased_ctls_t pin = {
            .val = vmread32(VMCS_CTRL_PINBASED_CONTROLS)
        };

        vmx_exit_ctls_t exit = {
            .val = vmread32(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS)
        };

        pin.fields.activate_vmx_preemption_timer = 1;
        exit.fields.save_vmx_preemption_timer = 1;

        __vmwrite(VMCS_CTRL_PINBASED_CONTROLS, pin.val);
        __vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, exit.val);
            
    } else {

        u32 rem = vmread32(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE);

        struct delta_node *current_timer = delta_chain_peekfront(chain);
        if (ticks < rem)
            __vmwrite(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, ticks);

        current_timer->delta = rem;
    }

    vcurrent_vcpu_enable_preemption();

    vtimer->timer.fields.vector = vector;
    vtimer->timer.fields.periodic = periodic;
    vtimer->timer.fields.armed = true;
    vtimer->timer.fields.nmi = nmi;
    vtimer->ticks = ticks;

    delta_chain_insert(chain, &vtimer->node, ticks);
    return 0;
}

/* long VDISARM_TIMERN(u8 timer_n) */
static long vdisarm_timern(struct vregs *vregs)
{
    u8 timer_n = vcall_arg1_8(vregs);

    if (timer_n >= VNUM_VTIMERS) {

        vcurrent_vcpu_enable_preemption();
        return -EINVAL;
    }

    struct vcpu *current = vcurrent_vcpu();
    struct vtimer *vtimer = &current->timers[timer_n];

    if (!vtimer->timer.fields.armed) {

        vcurrent_vcpu_enable_preemption();
        return -EINVAL;
    }

    struct delta_chain *chain = &current->timer_chain;
    struct delta_node *node = &vtimer->node;

    if (delta_chain_is_front(chain, node)) {

        if (!node->node.next) {

            vmx_pinbased_ctls_t pin = {
                .val = vmread32(VMCS_CTRL_PINBASED_CONTROLS)
            };

            vmx_exit_ctls_t exit = {
                .val = vmread32(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS)
            };
           
            pin.fields.activate_vmx_preemption_timer = 0;
            exit.fields.save_vmx_preemption_timer = 0;

            __vmwrite(VMCS_CTRL_PINBASED_CONTROLS, pin.val);
            __vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, exit.val); 

        } else {

            struct delta_node *next_node = node_to_delta_node(node->node.next);
            
            u32 rem = vmread32(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE);

            u64 next_delta = rem + next_node->delta;
            __vmwrite(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, next_delta);

            node->delta = rem;
        }
    }

    vcurrent_vcpu_enable_preemption();
    delta_chain_dequeue_no_callback(chain, node);

    vtimer->timer.fields.armed = false;
    return 0;
}

/* long VALTER_VCPU_TIMESLICE(u8 vid, u32 processor_id, u32 ticks) */
static long valter_vcpu_timeslice(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vid = vcall_arg1_8(vregs);
    u32 processor_id = vcall_arg2_32(vregs);
    u32 ticks = vcall_arg3_32(vregs);

    struct vcpu *current = vcurrent_vcpu();
    if (!current->root)
        return -EPERM;

    return vemu_alter_vcpu(vid, processor_id, true, ticks, false, 0);
}

/* long VALTER_VCPU_CRITICALITY(u8 vid, u32 processor_id, u8 criticality) */
static long valter_vcpu_criticality(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vid = vcall_arg1_8(vregs);
    u32 processor_id = vcall_arg2_32(vregs);
    u8 criticality = vcall_arg3_8(vregs);

    struct vcpu *current = vcurrent_vcpu();
    if (!current->root)
        return -EPERM;

    return vemu_alter_vcpu(vid, processor_id, false, 0, true, criticality);
}

/* long VIS_SERVICED(u8 intl) */
static long vis_serviced(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    struct vcpu *current = vcurrent_vcpu();

    u8 intl = vcall_arg1_8(vregs);

    if (intl > INTL_MAX)
        return -EINVAL;

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&current->visr_pending.lock, &node);

    u32 in_intl = current->visr_pending.delivery.fields.in_intl;
    bool serviced = ((in_intl >> intl) & 1) != 0;

    vmcs_unlock_isr_restore(&current->visr_pending.lock, &node);

    return serviced ? 1 : 0;
}

/* long VIS_PENDING(u8 vector, bool nmi) */
static long vis_pending(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vector = vcall_arg1_8(vregs);
    bool nmi = vcall_arg2_32(vregs);

    if (nmi && vector != NMI)
        return -EINVAL;

    struct vcpu *current = vcurrent_vcpu();

    bool is_pending = false;

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&current->visr_pending.lock, &node);
    
    if (nmi) {

        is_pending = current->visr_pending.delivery.fields.nmi_pending != 0;

    } else {
        struct bmp256 *pending = 
            &current->visr_pending.pending_external_interrupts;

        is_pending = bmp256_test(pending, vector);
    }

    vmcs_unlock_isr_restore(&current->visr_pending.lock, &node);

    return is_pending ? 1 : 0;
}

/* long VEOI(void) */
static long veoi(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    struct vcpu *current = vcurrent_vcpu();
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&current->visr_pending.lock, &node);

    u32 in_intl = current->visr_pending.delivery.fields.in_intl;
    int serviced_intl = fls32(in_intl);

    if (serviced_intl != -1) {
        u32 mask = ~(1 << serviced_intl);
        current->visr_pending.delivery.fields.in_intl &= mask;
    }

    vmcs_unlock_isr_restore(&current->visr_pending.lock, &node);

    return 0;
}

#if CONFIG_TWANVISOR_VSCHED_STRICT

/* long VYIELD(void) */
static long vyield(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    return -EINVAL;
}

/* long VPAUSE(void) */
static long vpause(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    return -EINVAL;
}

#else

/* long VYIELD(void) */
static long vyield(__unused struct vregs *vregs)
{
    vsched_yield();
    vcurrent_vcpu_enable_preemption();

    return 0;
}

/* long VPAUSE(void) */
static long vpause(__unused struct vregs *vregs)
{
    vsched_pause(false);
    vcurrent_vcpu_enable_preemption();

    return 0;
}

#endif

/* long VREAD_VCPU_STATE(u32 processor_id) */
static long vread_vcpu_state(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u32 processor_id = vcall_arg1_32(vregs);
    return vemu_read_vcpu_state_local(processor_id);
}

/* long VREAD_VCPU_STATE_FAR(u8 vid, u32 processor_id) */
static long vread_vcpu_state_far(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vid = vcall_arg1_8(vregs);
    u32 processor_id = vcall_arg2_32(vregs);

    return vemu_read_vcpu_state_far(vid, processor_id);
}

/* long VSET_ROUTE(u8 target_vid, u8 sender_vid, u32 route_type, bool allow) */
static long vset_route(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    struct vcpu *vcpu = vcurrent_vcpu();

    u8 target_vid = vcall_arg1_8(vregs);
    u8 sender_vid = vcall_arg2_8(vregs);
    u32 route_type = vcall_arg3_32(vregs);
    bool allow = vcall_arg4_32(vregs);

    if (!vcpu->root)
        return -EPERM;
    
    if (sender_vid == vcpu->vid)
        return -EINVAL;

    return vemu_set_route(target_vid, sender_vid, route_type, allow);
}

/* long VSET_VCPU_SUBSCRIPTION_PERM(u8 vid, u32 processor_id, u8 vector, 
                                    bool allow) */
static long vset_vcpu_subscription_perm(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    struct vcpu *vcpu = vcurrent_vcpu();

    u8 vid = vcall_arg1_8(vregs);
    u32 processor_id = vcall_arg2_32(vregs);
    u8 vector = vcall_arg3_8(vregs);
    bool allow = vcall_arg4_32(vregs);

    if (!vcpu->root)
        return -EPERM;
    
    if (vid == vcpu->vid)
        return -EINVAL;

    return vemu_set_vcpu_subscription_perms(vid, processor_id, 
                                            vector, allow);
}

/* long VSET_CRITICALITY_PERM(u8 vid, u32 processor_id,
                              vcriticality_perm_t perm) */
static long vset_criticality_perm(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    struct vcpu *vcpu = vcurrent_vcpu();

    u8 vid = vcall_arg1_8(vregs);
    u32 phys_processor_id = vcall_arg2_32(vregs);
    vcriticality_perm_t perm = {.val = vcall_arg3_32(vregs)};

    if (!vcpu->root)
        return -EPERM;
    
    if (vid == vcpu->vid)
        return -EINVAL;

    return vemu_set_criticality_perm(vid, phys_processor_id, perm);
}

/* long VREAD_CRITICALITY_LEVEL(int physical_processor_id) */
static long vread_criticality_level(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vid = vcurrent_vcpu()->vid;

    int phys_processor_id = vcall_arg1_32(vregs);

    if (phys_processor_id < 0)
        phys_processor_id = vthis_vprocessor_id();

    return vemu_read_criticality_level(vid, phys_processor_id);
}

/* long VWRITE_CRITICALITY_LEVEL(int physical_processor_id, 
                                 u8 criticality_level) */
static long vwrite_criticality_level(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vid = vcurrent_vcpu()->vid;

    int phys_processor_id = vcall_arg1_32(vregs);
    u8 criticality = vcall_arg2_8(vregs);

    if (phys_processor_id < 0)
        phys_processor_id = vthis_vprocessor_id();

    return vemu_write_criticality_level(vid, phys_processor_id, 
                                        criticality);
}

#if CONFIG_TWANVISOR_VSCHED_STRICT

/* long VPV_SPIN_PAUSE(void) */
static long vpv_spin_pause(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    return -EINVAL;
}

/* long VPV_SPIN_KICK(u32 processor_id) */
static long vpv_spin_kick(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    return -EINVAL;
}

/* long VPV_SPIN_KICK_FAR(u8 vid, u32 processor_id) */
static long vpv_spin_kick_far(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    return -EINVAL;
}

#else

/* long VPV_SPIN_PAUSE(void) */
static long vpv_spin_pause(__unused struct vregs *vregs)
{
    vsched_pause(true);
    vcurrent_vcpu_enable_preemption();

    return 0;
}

/* long VPV_SPIN_KICK(u32 processor_id) */
static long vpv_spin_kick(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u32 processor_id = vcall_arg1_32(vregs);
    return vemu_pv_spin_kick_local(processor_id);
}

/* long VPV_SPIN_KICK_FAR(u8 vid, u32 processor_id) */
static long vpv_spin_kick_far(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vid = vcall_arg1_8(vregs);
    u32 processor_id = vcall_arg2_32(vregs);

    return vemu_pv_spin_kick_far(vid, processor_id);
}

#endif

/* long VKDBG(const char *str) */
static long vkdbg(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    char *str = (void *)vcall_arg1_64(vregs);

    struct vcpu *vcpu = vcurrent_vcpu();
    if (!vcpu->root)
        return -EPERM;

    if (!str)
        return -EINVAL;

    vdbgf("%s", str);
    return 0;
}

/* long VCREATE_PARTITION(struct vpartition *vpartition) */
static long vcreate_partition(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    struct vcpu *vcpu = vcurrent_vcpu();
    if (!vcpu->root)
        return -EPERM;

    struct vpartition *vpartition = (void *)vcall_arg1_64(vregs);

    int err = vpartition_precheck(vpartition);
    if (err < 0)
        return err;

    int vid = vpartition_id_alloc();
    if (vid < 0)
        return -ENOSPC;

    vpartition_setup(vpartition, vid);
    vpartition_push(vpartition);

    return vid;
}

/* long VDESTROY_PARTITION(u8 vid) */
static long vdestroy_partition(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vid = vcall_arg1_8(vregs);

    struct vcpu *vcpu = vcurrent_vcpu();
    if (!vcpu->root)
        return -EPERM;

    if (vid == vcpu->vid)
        return -EINVAL;

    return vteardown(vid);
}

#if CONFIG_TWANVISOR_VSCHED_MCFS

/* long VFRAME_SET(u8 vid, u32 processor_id, u32 frame_id) */
static long vframe_set(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u8 vid = vcall_arg1_8(vregs);
    u32 processor_id = vcall_arg2_32(vregs);
    u32 frame_id = vcall_arg3_32(vregs);

    struct vcpu *vcpu = vcurrent_vcpu();
    if (!vcpu->root)
        return -EPERM;

    return vemu_vframe_set(vid, processor_id, frame_id);
}

/* long VFRAME_UNSET(int physical_processor_id, u32 frame_id) */
static long vframe_unset(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    int phys_processor_id = vcall_arg1_32(vregs);
    u32 frame_id = vcall_arg2_32(vregs);

    if (phys_processor_id < 0)
        phys_processor_id = vthis_vprocessor_id();

    struct vcpu *vcpu = vcurrent_vcpu();
    if (!vcpu->root)
        return -EPERM;

    return vemu_vframe_unset(phys_processor_id, frame_id);
}

#else

/* long VFRAME_SET(u8 vid, u32 processor_id, u32 frame_id) */
static long vframe_set(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    return -EINVAL;
}

/* long VFRAME_UNSET(int physical_processor_id, u32 frame_id) */
static long vframe_unset(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    return -EINVAL;
}

#endif

static vcall_func_t vcall_table[] = {

    [VSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR] = 
        vsubscribe_external_interrupt_vector,

    [VUNSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR] = 
        vunsubscribe_external_interrupt_vector,

    [VIPI] = vipi,
    [VIPI_FAR] = vipi_far,
    [VTLB_SHOOTDOWN] = vtlb_shootdown,

    [VARM_TIMERN] = varm_timern,
    [VDISARM_TIMERN] = vdisarm_timern,

    [VALTER_VCPU_TIMESLICE] = valter_vcpu_timeslice,
    [VALTER_VCPU_CRITICALITY] = valter_vcpu_criticality,

    [VIS_SERVICED] = vis_serviced,
    [VIS_PENDING] = vis_pending,
    [VEOI] = veoi,

    [VYIELD] = vyield,
    [VPAUSE] = vpause,

    [VREAD_VCPU_STATE] = vread_vcpu_state,
    [VREAD_VCPU_STATE_FAR] = vread_vcpu_state_far,

    [VSET_ROUTE] = vset_route,
    [VSET_VCPU_SUBSCRIPTION_PERM] = vset_vcpu_subscription_perm,
    [VSET_CRITICALITY_PERM] = vset_criticality_perm,

    [VREAD_CRITICALITY_LEVEL] = vread_criticality_level,
    [VWRITE_CRITICALITY_LEVEL] = vwrite_criticality_level,

    [VPV_SPIN_PAUSE] = vpv_spin_pause,
    [VPV_SPIN_KICK] = vpv_spin_kick,
    [VPV_SPIN_KICK_FAR] = vpv_spin_kick_far,

    [VKDBG] = vkdbg,

    [VCREATE_PARTITION] = vcreate_partition,
    [VDESTROY_PARTITION] = vdestroy_partition,

    [VFRAME_SET] = vframe_set,
    [VFRAME_UNSET] = vframe_unset,
};

void vcall_dispatcher(struct vregs *vregs)
{
    vop_mode_t mode;
    if (!vis_guest_cpl0(&mode)) {

        vcurrent_vcpu_enable_preemption();
        vqueue_inject_gp0();
        return;
    }

    long ret;

    u64 id = vcall_id(vregs);
    if (id >= ARRAY_LEN(vcall_table)) {

        vcurrent_vcpu_enable_preemption();
        ret = -EINVAL;
        
    } else {

        vcall_func_t func = vcall_table[id];
        VDYNAMIC_ASSERT(func);

        ret = INDIRECT_BRANCH_SAFE(func(vregs));
    }

    if (mode != VOP_64_BIT)
        ret &= 0xffffffff;

    vcall_set_retval(vregs, ret);
}

#endif