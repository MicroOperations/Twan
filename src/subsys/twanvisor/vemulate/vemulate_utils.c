#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vemulate/vemulate_utils.h>
#include <subsys/twanvisor/twanvisor.h>
#include <subsys/twanvisor/vsched/vpartition.h>
#include <subsys/twanvisor/vsched/vsched_mcs.h>
#include <subsys/twanvisor/vsched/vsched_yield.h>

u64 vlapic_read(u32 offset)
{
    if (vthis_cpu_data()->arch_flags.support.fields.x2apic == 0) 
        return *(volatile u32 *)((u8 *)vtwan()->lapic_mmio + offset);

    u32 msr = IA32_X2APIC_BASE + (offset / 16);
    return __rdmsrl(msr);   
}

void vlapic_write(u32 offset, u64 val)
{
    if (vthis_cpu_data()->arch_flags.support.fields.x2apic == 0) {

        u32 lower = (u32)val;
        u32 upper = (u32)(val >> 32);

        u64 base = (u64)vtwan()->lapic_mmio;

        if (offset == LAPIC_ICR_HIGH_OFFSET) {
            *(volatile u32 *)(base + offset) = upper;
            *(volatile u32 *)(base + LAPIC_ICR_LOW_OFFSET) = lower;
            return;
        }

        *(volatile u32 *)(base + offset) = lower;
        return;
    }

    if (offset == LAPIC_ICR_HIGH_OFFSET)
        offset = LAPIC_ICR_LOW_OFFSET;

    u32 msr = IA32_X2APIC_BASE + (offset / 16);
    __wrmsrl(msr, val);
}

void vtrap_msr_write(struct vcpu *vcpu, u32 msr)
{
    int base = 0;
    int idx = 0;
    vmap_msr_write(msr, &base, &idx);

    if (base != -1 && idx != -1)
        vcpu->arch.msr_bitmap[base + (idx / 8)] |= (1 << (idx % 8));
}

void vtrap_msr_read(struct vcpu *vcpu, u32 msr)
{
    int base = 0;
    int idx = 0;
    vmap_msr_read(msr, &base, &idx);

    if (base != -1 && idx != -1)
        vcpu->arch.msr_bitmap[base + (idx / 8)] |= (1 << (idx % 8));
}

void vtrap_msr(struct vcpu *vcpu, u32 msr)
{
    vtrap_msr_read(vcpu, msr);
    vtrap_msr_write(vcpu, msr);
}

void __vemu_set_interrupt_pending(struct vcpu *vcpu, u8 vector, bool nmi)
{
    if (nmi)
        vcpu->visr_pending.delivery.fields.nmi_pending = 1;
    else
        bmp256_set(&vcpu->visr_pending.pending_external_interrupts, vector);
}

int vemu_set_interrupt_pending(struct vcpu *vcpu, u8 vector, bool nmi)
{
    if (nmi && vector != NMI)
        return -EINVAL;

    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vcpu->visr_pending.lock, &node);
    __vemu_set_interrupt_pending(vcpu, vector, nmi);
    vmcs_unlock_isr_restore(&vcpu->visr_pending.lock, &node);
    
    return 0;
}

bool vemu_is_interrupt_pending(struct vcpu *vcpu, u8 vector, bool nmi)
{
    struct mcsnode pending_node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vcpu->visr_pending.lock, &pending_node);

    struct bmp256 *pending = &vcpu->visr_pending.pending_external_interrupts;
    bool nmi_pending = vcpu->visr_pending.delivery.fields.nmi_pending != 0;

    bool ret = nmi ? nmi_pending : bmp256_test(pending, vector);

    vmcs_unlock_isr_restore(&vcpu->visr_pending.lock, &pending_node);

    return ret;
}

int __vemu_unpause_vcpu(struct vcpu *vcpu, bool inject, u8 vector, bool nmi)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);
    struct mcsnode vsched_node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vsched->lock, &vsched_node);

    if (vcpu->vsched_metadata.state != VPAUSED) {
        vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);
        return -EINVAL;
    }

    __vsched_dequeue_paused(vcpu);

    vcpu->vsched_metadata.state = VREADY;
    __vsched_push(vcpu);

    int ret = inject ? vemu_set_interrupt_pending(vcpu, vector, nmi) : 0;

    vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);
    return ret;
}

int vemu_unpause_vcpu_local(u32 processor_id, bool inject, u8 vector, bool nmi)
{
    if (inject && nmi && vector != NMI)
        return -EINVAL;

    struct vcpu *current = vcurrent_vcpu();
    if (processor_id >= current->vpartition_num_cpus)
        return -EINVAL;

    u8 vid = current->vid;

    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(vid, &vpartition_node);

    if (!vpartition)
        return -EFAULT;

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    int ret = __vemu_unpause_vcpu(vcpu, inject, vector, nmi);
        
    vpartition_put(vid, &vpartition_node);
    return ret;
}

int vemu_unpause_vcpu_far(u8 target_vid, u32 processor_id, bool inject, 
                          u8 vector, bool nmi)
{
    u8 sender_vid = vcurrent_vcpu()->vid;

    if (sender_vid == target_vid || (inject && nmi && vector != NMI))
        return -EINVAL;

    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();

    struct vpartition *vpartition = 
        vpartition_get(target_vid, &vpartition_node);

    if (!vpartition)
        return -EINVAL;

    if (!bmp256_test(&vpartition->ipi_senders, sender_vid) || 
        processor_id >= vpartition->vcpu_count) {

        vpartition_put(target_vid, &vpartition_node);
        return -EINVAL;
    }

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    int ret = __vemu_unpause_vcpu(vcpu, inject, vector, nmi);

    vpartition_put(target_vid, &vpartition_node);
    return ret;
}

void __vemu_inject_external_interrupt(struct vcpu *vcpu, u8 vector, bool nmi)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);
    u32 vprocessor_id = vqueue_to_vprocessor_id(vcpu->vqueue_id);

    struct mcsnode vsched_node = INITIALIZE_MCSNODE();
    struct mcsnode pending_node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vsched->lock, &vsched_node);
    vmcs_lock_isr_save(&vcpu->visr_pending.lock, &pending_node);

    vinterrupt_delivery_data_t data = vcpu->visr_pending.delivery;

    bool nmi_pending = data.fields.nmi_pending != 0;
    bool nmi_window_exiting = data.fields.nmi_window_exit != 0;
    bool int_window_exiting = data.fields.int_window_exit != 0;

    struct bmp256 *pending_interrupts = 
        &vcpu->visr_pending.pending_external_interrupts;

    bool can_inject = nmi ? !nmi_pending : 
                            !bmp256_test(pending_interrupts, vector);

    if (can_inject) {

        /* not even worth deqeuing from its dispatcher once setting it as 
           pending here, due to overhead from contending for the dispatcher
           lock */

        __vemu_set_interrupt_pending(vcpu, vector, nmi);

        /* check if we should force a vmexit for the interrupt to be injected if 
           the vcpu is running */

        if (vcpu->vsched_metadata.state == VRUNNING) {

            if (nmi) {

                if (data.fields.in_nmi == 0 || !nmi_window_exiting)
                    vipi_async(vprocessor_id);

            } else if (!int_window_exiting) {

                u32 in_intl = data.fields.in_intl;
                int current_intl = data.fields.intl;

                int serviced_intl = fls32(in_intl);
                int intl = vector_to_intl(vector);

                if (intl > serviced_intl &&  intl >= current_intl)
                    vipi_async(vprocessor_id);
            }
        } 
    }

    vmcs_unlock_isr_restore(&vcpu->visr_pending.lock, &pending_node);
    vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);
}

int vemu_inject_external_interrupt_local(u32 processor_id, u8 vector, bool nmi)
{
    if (nmi && vector != NMI)
        return -EINVAL;

    struct vcpu *current = vcurrent_vcpu();
    if (processor_id >= current->vpartition_num_cpus)
        return -EINVAL;

    if (current->processor_id == processor_id)
        return vemu_set_interrupt_pending(current, vector, nmi);

    u8 vid = current->vid;

    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();

    struct vpartition *vpartition = vpartition_get(vid, &vpartition_node);
    if (!vpartition)
        return -EFAULT;

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    __vemu_inject_external_interrupt(vcpu, vector, nmi);

    vpartition_put(vid, &vpartition_node);
    return 0;
}

int vemu_inject_external_interrupt_far(u8 target_vid, u32 processor_id, 
                                       u32 vector, bool nmi)
{
    u8 sender_vid = vcurrent_vcpu()->vid;

    if (sender_vid == target_vid || (nmi && vector != NMI))
        return -EINVAL;

    struct mcsnode node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(target_vid, &node);
    if (!vpartition)
        return -EINVAL;

    if (!bmp256_test(&vpartition->ipi_senders, sender_vid) ||
        processor_id >= vpartition->vcpu_count) {

        vpartition_put(target_vid, &node);
        return -EINVAL;
    }

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    __vemu_inject_external_interrupt(vcpu, vector, nmi);

    vpartition_put(target_vid, &node);
    return 0;
}

static u8 pending_maps[NUM_CPUS][NUM_CPUS];

int vemu_tlb_invalidate(u8 target_vid)
{   
    struct vcpu *current = vcurrent_vcpu();
    u8 sender_vid = current->vid;

    struct mcsnode node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(target_vid, &node);

    if (!vpartition)
        return -EINVAL;

    if (!bmp256_test(&vpartition->tlb_shootdown_senders, sender_vid) && 
        sender_vid != target_vid) {

        vpartition_put(target_vid, &node);
        return -EINVAL;
    }

    u32 this_vprocessor_id = vthis_vprocessor_id();
    u32 num_enabled_cpus = vnum_cpus();

    u8 *map = pending_maps[this_vprocessor_id];
    memset(map, 0, num_enabled_cpus);

    /* first pass: set relevant vcpus tlb flush pending state, if theyre running
       then queue us up and mark them to be ipi'd */
    for (u32 i = 0; i < vpartition->vcpu_count; i++) {

        struct vcpu *vcpu = &vpartition->vcpus[i];
        u32 vprocessor_id = vqueue_to_vprocessor_id(vcpu->vqueue_id);

        struct vscheduler *vsched = vscheduler_of(vcpu);
        struct mcsnode vsched_node = INITIALIZE_MCSNODE();

        vmcs_lock_isr_save(&vsched->lock, &vsched_node);

        vcpu_state_t state = vcpu->vsched_metadata.state;

        bool was_pending = vcpu->vsched_metadata.tlb_flush_pending;
        vcpu->vsched_metadata.tlb_flush_pending = true;

        VBUG_ON(vcpu == current && state != VTRANSITIONING);

        if (state == VRUNNING) {

            vipi_queue_ack(vprocessor_id, current);

            if (!was_pending)
                vipi_async(vprocessor_id);

            map[vprocessor_id] = 1;
        }

        vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);
    }

    vpartition_put(target_vid, &node);

    VBUG_ON(!vcurrent_vcpu_is_preemption_enabled());
    VBUG_ON(!is_interrupts_enabled());

    /* second pass: wait for target processors to ack */
    for (u32 i = 0; i < num_enabled_cpus; i++) {
        if (map[i] == 1)
            spin_until(vipi_check_ack(i, current) == 1);
    }

    return 0;
}

int vemu_alter_vcpu(u8 vid, u32 processor_id, bool alter_ticks, u32 ticks, 
                    bool alter_criticality, u8 criticality)
{
    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();

    struct vpartition *vpartition = vpartition_get(vid, &vpartition_node);
    if (!vpartition)
        return -EINVAL;

    if (processor_id >= vpartition->vcpu_count) {
        vpartition_put(vid, &vpartition_node);
        return -EINVAL;
    }

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    u32 vprocessor_id = vqueue_to_vprocessor_id(vcpu->vqueue_id);
    struct vscheduler *vsched = &vper_cpu_data(vprocessor_id)->vscheduler;

    struct mcsnode sched_node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vsched->lock, &sched_node);

    /* if the vcpu is in a state where it is apreemptive (i.e, its timeslice 
       is 0) then we need to force it to rearm the timer if its running, it'll
       vmexit and rearm the timer, live out the rest of its timeslice before
       being preempted */

    if (alter_ticks) {

        vcpu->vsched_metadata.time_slice_ticks = ticks;

        u32 curent_ticks = vcpu->vsched_metadata.current_time_slice_ticks;
        if (curent_ticks == 0 && ticks != 0) {

            vcpu->vsched_metadata.current_time_slice_ticks = ticks;

            if (vcpu->vsched_metadata.state == VRUNNING) {
                vcpu->vsched_metadata.rearm = true;
                vipi_async(vprocessor_id);
                
            } else if (vcpu == vcurrent_vcpu()) {
                vcpu->vsched_metadata.rearm = true;
            }
        }
    }

    if (alter_criticality && vcpu->vsched_metadata.criticality != criticality) {

        if (vcpu->vsched_metadata.state == VREADY) {

            __vsched_dequeue(vcpu);
            vcpu->vsched_metadata.criticality = criticality;
            __vsched_push(vcpu);

        } else {
            vcpu->vsched_metadata.criticality = criticality;
        }
    }

    vmcs_unlock_isr_restore(&vsched->lock, &sched_node);

    vpartition_put(vid, &vpartition_node);
    return 0;
}

vcpu_state_t __vemu_read_vcpu_state(struct vcpu *vcpu)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vsched->lock, &node);
    vcpu_state_t state = vcpu->vsched_metadata.state;
    vmcs_unlock_isr_restore(&vsched->lock, &node);

    return state;
}

int vemu_read_vcpu_state_local(u32 processor_id)
{
    struct vcpu *current = vcurrent_vcpu();
    if (processor_id >= current->vpartition_num_cpus)
        return -EINVAL;

    u8 vid = current->vid;

    struct mcsnode node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(vid, &node);
    if (!vpartition)
        return -EFAULT;

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    vcpu_state_t state = __vemu_read_vcpu_state(vcpu);

    vpartition_put(vid, &node);
    return state;
}

int vemu_read_vcpu_state_far(u8 target_vid, u32 processor_id)
{
    u8 sender_vid = vcurrent_vcpu()->vid;
    if (target_vid == sender_vid)
        return -EINVAL;

    struct mcsnode node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(target_vid, &node);
    if (!vpartition)
        return -EINVAL;

    if (!bmp256_test(&vpartition->read_vcpu_state_senders, sender_vid) ||
        processor_id >= vpartition->vcpu_count) {

        vpartition_put(target_vid, &node);
        return -EINVAL;
    }

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    vcpu_state_t state = __vemu_read_vcpu_state(vcpu);

    vpartition_put(target_vid, &node);
    return state;
}

int vemu_set_route(u8 target_vid, u8 sender_vid, vroute_type_t route_type, 
                   bool allow)
{
    if (sender_vid == target_vid || route_type >= VNUM_ROUTE_TYPES)
        return -EINVAL;

    u8 vid1 = min(sender_vid, target_vid);
    u8 vid2 = max(sender_vid, target_vid);

    struct mcsnode node1 = INITIALIZE_MCSNODE();
    struct mcsnode node2 = INITIALIZE_MCSNODE();

    struct vpartition *vpartition1 = vpartition_get(vid1, &node1);
    if (!vpartition1)
        return -EINVAL;

    struct vpartition *vpartition2 = vpartition_get(vid2, &node2);
    if (!vpartition2) {
        vpartition_put(vid1, &node1);
        return -EINVAL;
    }

    struct vpartition *sender = vid1 == sender_vid ? vpartition1 : vpartition2;
    struct vpartition *target = vid1 == target_vid ? vpartition1 : vpartition2;

    struct bmp256 *sender_receivers_map = NULL;
    struct bmp256 *target_senders_map = NULL;

    switch (route_type) {
        
        case VIPI_ROUTE:

            sender_receivers_map = &sender->ipi_receivers;
            target_senders_map = &target->ipi_senders;
            break;

        case VTLB_SHOOTDOWN_ROUTE:

            sender_receivers_map = &sender->tlb_shootdown_receivers;
            target_senders_map = &target->tlb_shootdown_senders;
            break;

        case VREAD_VCPU_STATE_ROUTE:

            sender_receivers_map = &sender->read_vcpu_state_receivers;
            target_senders_map = &target->read_vcpu_state_senders;
            break;

        case VPV_SPIN_KICK_ROUTE:

            sender_receivers_map = &sender->pv_spin_kick_receivers;
            target_senders_map = &target->pv_spin_kick_senders;
            break;

        default:
            VBUG_ON(true);
            break;
    }

    if (allow) {
        bmp256_set(sender_receivers_map, target_vid);
        bmp256_set(target_senders_map, sender_vid);
    } else {
        bmp256_unset(sender_receivers_map, target_vid);
        bmp256_unset(target_senders_map, sender_vid);
    }

    vpartition_put(vid2, &node2);
    vpartition_put(vid1, &node1);

    return 0;
}

int vemu_set_vcpu_subscription_perms(u8 vid, u32 processor_id, u8 vector,
                                     bool allow)
{
    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(vid, &vpartition_node);
    if (!vpartition)
        return -EINVAL;

    if (processor_id >= vpartition->vcpu_count) {
        vpartition_put(vid, &vpartition_node);
        return -EINVAL;
    }

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    u32 vprocessor_id = vqueue_to_vprocessor_id(vcpu->vqueue_id);
    
    struct vper_cpu *vper_cpu = vper_cpu_data(vprocessor_id);
    if (!bmp256_test(&vper_cpu->available_vectors, vector)) {
        vpartition_put(vid, &vpartition_node);
        return -EINVAL;
    }

    struct mcsnode dispatch_node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vper_cpu->dispatch_lock, &dispatch_node);

    if (allow) {

        bmp256_set(&vcpu->visr_metadata.allowed_external_vectors, vector);

    } else {

        struct dq *dq = &vper_cpu->dispatch_vectors[vector];
        struct list_double *dispatch_node = &vcpu->vdispatch_nodes[vector];

        bmp256_unset(&vcpu->visr_metadata.allowed_external_vectors, vector);
        bmp256_unset(&vcpu->visr_metadata.subscribed_vectors, vector);

        if (dq_is_queued(dq, dispatch_node))
            dq_dequeue(dq, dispatch_node);
    }

    vmcs_unlock_isr_restore(&vper_cpu->dispatch_lock, &dispatch_node);

    vpartition_put(vid, &vpartition_node);
    return 0;
}

int vemu_set_criticality_perm(u8 vid, u32 physical_processor_id, 
                              vcriticality_perm_t perm)
{
    u8 min = perm.fields.min_criticality_writeable;
    u8 max = perm.fields.max_criticality_writeable;

    if (physical_processor_id >= vnum_cpus())
        return -EINVAL;

    if (perm.fields.write != 0 && (min > max || max > VSCHED_MAX_CRITICALITY))
        return -EINVAL;

    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(vid, &vpartition_node);
    if (!vpartition)
        return -EINVAL;

    vpartition->criticality_perms[physical_processor_id] = perm;

    vpartition_put(vid, &vpartition_node);
    return 0;
}

int vemu_read_criticality_level(u8 requester_vid, u32 physical_processor_id)
{
    if (physical_processor_id >= vnum_cpus())
        return -EINVAL;

    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(requester_vid, 
                                                   &vpartition_node);
    if (!vpartition)
        return -EINVAL;

    vcriticality_perm_t perm = 
        vpartition->criticality_perms[physical_processor_id];

    int ret = perm.fields.read != 0 ? 
              vsched_mcs_read_criticality_level(physical_processor_id) : -EPERM;

    vpartition_put(requester_vid, &vpartition_node);
    return ret;
}

int vemu_write_criticality_level(u8 requester_vid, u32 physical_processor_id, 
                                 u8 criticality_level)
{
    if (physical_processor_id >= vnum_cpus() || 
        criticality_level > VSCHED_MAX_CRITICALITY) {

        return -EINVAL;
    }

    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(requester_vid, 
                                                   &vpartition_node);
    if (!vpartition)
        return -EINVAL;

    vcriticality_perm_t perm = 
        vpartition->criticality_perms[physical_processor_id];

    int ret = -EINVAL;

    u8 min = perm.fields.min_criticality_writeable;
    u8 max = perm.fields.max_criticality_writeable;

    if (perm.fields.write && criticality_level >= min && 
        criticality_level <= max) {

        ret = vsched_mcs_write_criticality_level(physical_processor_id, 
                                                 criticality_level);
    }

    vpartition_put(requester_vid, &vpartition_node);
    return ret;
}

void __vemu_pv_spin_kick(struct vcpu *vcpu)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);

    struct mcsnode vsched_node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vsched->lock, &vsched_node);

    vcpu_pv_spin_state_t state = vcpu->vsched_metadata.pv_spin_state;

    if (state == VSPIN_PAUSED) {

        VBUG_ON(vcpu->vsched_metadata.state != VPV_SPINNING);

        __vsched_dequeue_paused(vcpu);

        vcpu->vsched_metadata.state = VREADY;
        __vsched_push(vcpu);

        vcpu->vsched_metadata.pv_spin_state = VSPIN_NONE;
        
    } else {
        vcpu->vsched_metadata.pv_spin_state = VSPIN_KICKED;
    }

    vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);
}

int vemu_pv_spin_kick_local(u32 processor_id)
{   
    u8 vid = vcurrent_vcpu()->vid;

    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(vid, &vpartition_node);

    if (!vpartition)
        return -EINVAL;

    if (processor_id >= vpartition->vcpu_count) {
        vpartition_put(vid, &vpartition_node);
        return -EINVAL;
    }

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    __vemu_pv_spin_kick(vcpu);

    vpartition_put(vid, &vpartition_node);
    return 0;
}

int vemu_pv_spin_kick_far(u8 target_vid, u32 processor_id)
{
    u8 sender_vid = vcurrent_vcpu()->vid;
    if (target_vid == sender_vid)
        return -EINVAL;

    struct mcsnode node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(target_vid, &node);
    if (!vpartition)
        return -EINVAL;

    if (!bmp256_test(&vpartition->pv_spin_kick_senders, sender_vid) ||
        processor_id >= vpartition->vcpu_count) {

        vpartition_put(target_vid, &node);
        return -EINVAL;
    }

    struct vcpu *vcpu = &vpartition->vcpus[processor_id];
    __vemu_pv_spin_kick(vcpu);

    vpartition_put(target_vid, &node);
    return 0;
}

#endif