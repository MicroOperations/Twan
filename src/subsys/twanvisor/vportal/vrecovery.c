#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/subsys/twanvisor/vportal/vrecovery.h>
#include <include/subsys/twanvisor/vsched/vsched_yield.h>
#include <include/subsys/twanvisor/vemulate/vemulate_utils.h>
#include <include/subsys/twanvisor/vemulate/verror.h>

void vdispatcher_dequeue(struct vper_cpu *target, struct vcpu *vcpu)
{
    struct mcsnode dispatch_node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&target->dispatch_lock, &dispatch_node);

    for (u32 i = 0; i < ARRAY_LEN(vcpu->vdispatch_nodes); i++) {
        
        struct dq *dq = &target->dispatch_vectors[i];
        struct list_double *node = &vcpu->vdispatch_nodes[i];

        if (dq_is_queued(dq, node))
            dq_dequeue(dq, node);
    }

    vmcs_unlock_isr_restore(&target->dispatch_lock, &dispatch_node);
}

void vipi_ack_dequeue_all(struct vcpu *vcpu)
{
    for (u32 i = 0; i < ARRAY_LEN(vcpu->ipi_nodes); i++) {

        struct vper_cpu *target = vper_cpu_data(i);

        struct mcsnode ipi_node = INITIALIZE_MCSNODE();
        vmcs_lock_isr_save(&target->vipi_data.vcpus.lock, &ipi_node);

        struct dq *dq = &target->vipi_data.vcpus.vcpu_sender_dq;
        struct list_double *node = &vcpu->ipi_nodes[i];

        if (dq_is_queued(dq, node))
            dq_dequeue(dq, node);

        vmcs_unlock_isr_restore(&target->vipi_data.vcpus.lock, &ipi_node);
    }
}

void vdo_finalize_teardown(u8 vid)
{
    u8 root_vid = vtwan()->root_vid;

    vpartition_start_exclusive();

    struct vpartition *vpartition = vpartition_get_exclusive(vid);
    VBUG_ON(!vpartition);

    /* unset the root partition here so we dont accidentally ruin its perms 
       during unwind */
    bmp256_unset(&vpartition->ipi_senders, root_vid); 
    bmp256_unset(&vpartition->tlb_shootdown_senders, root_vid); 
    bmp256_unset(&vpartition->read_vcpu_state_senders, root_vid);  
    
    u32 processor_id = vpartition->terminate_notification.processor_id;
    u8 vector = vpartition->terminate_notification.vector;
    bool nmi = vpartition->terminate_notification.nmi;

    int pair_vid;

    /* unwind ipi routes */
    while ((pair_vid = bmp256_fls(&vpartition->ipi_senders)) != -1) {

        struct vpartition *pair = vpartition_get_exclusive(pair_vid);
        VBUG_ON(!pair);

        bmp256_unset(&pair->ipi_receivers, vid);
        bmp256_unset(&vpartition->ipi_senders, pair_vid);
    }

    while ((pair_vid = bmp256_fls(&vpartition->ipi_senders)) != -1) {

        struct vpartition *pair = vpartition_get_exclusive(pair_vid);
        VBUG_ON(!pair);

        bmp256_unset(&pair->ipi_senders, vid);
        bmp256_unset(&vpartition->ipi_receivers, pair_vid);
    }

    /* unwind tlb shootdown routes */
    while ((pair_vid = bmp256_fls(&vpartition->tlb_shootdown_senders)) != -1) {

        struct vpartition *pair = vpartition_get_exclusive(pair_vid);
        VBUG_ON(!pair);

        bmp256_unset(&pair->tlb_shootdown_receivers, vid);
        bmp256_unset(&vpartition->tlb_shootdown_senders, pair_vid);
    }

    while (
        (pair_vid = bmp256_fls(&vpartition->tlb_shootdown_receivers)) != -1) {

        struct vpartition *pair = vpartition_get_exclusive(pair_vid);
        VBUG_ON(!pair);

        bmp256_unset(&pair->tlb_shootdown_senders, vid);
        bmp256_unset(&vpartition->tlb_shootdown_receivers, pair_vid);
    }

    /* unwind read vcpu state routes */
    while (
        (pair_vid = bmp256_fls(&vpartition->read_vcpu_state_senders)) != -1) {

        struct vpartition *pair = vpartition_get_exclusive(pair_vid);
        VBUG_ON(!pair);

        bmp256_unset(&pair->read_vcpu_state_receivers, vid);
        bmp256_unset(&vpartition->read_vcpu_state_senders, pair_vid);
    }

    while (
        (pair_vid = bmp256_fls(&vpartition->read_vcpu_state_receivers)) != -1) {

        struct vpartition *pair = vpartition_get_exclusive(pair_vid);
        VBUG_ON(!pair);

        bmp256_unset(&pair->read_vcpu_state_senders, vid);
        bmp256_unset(&vpartition->read_vcpu_state_receivers, pair_vid);
    }

    __vpartition_remove(vid);
    vpartition_stop_exclusive();

    vpartition_id_free(vid);

    /* inject an interrupt to root so it knows the vpartition has finished its
       teardown */
    struct mcsnode root_node = INITIALIZE_MCSNODE();
    struct vpartition *root = vpartition_get(root_vid, &root_node);
    VBUG_ON(!root);

    VBUG_ON(processor_id >= root->vcpu_count);
    struct vcpu *root_vcpu = &root->vcpus[processor_id];

    __vemu_inject_external_interrupt(root_vcpu, vector, nmi);

    vpartition_put(root_vid, &root_node);
}

void vfinalize_teardown_ipi(u64 vid)
{
    struct interrupt_info *ctx = vthis_cpu_data()->vcpu_ctx;
    vdo_finalize_teardown(vid);
    vsched_get_spin(ctx);
}

void vfinalize_teardown(u8 vid)
{
    vemulate_self_ipi(vfinalize_teardown_ipi, vid);
}

void vfailure_recover(void)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    struct vcpu *current = vcurrent_vcpu();
    
    disable_interrupts();

    if (current->root) {
        vpanicf_global("root failure\n");
        vpanicf_global("root processor id: %u\n", current->processor_id);
    }

    /* ensure stale timer interrupts dont corrupt the next vcpus timeslice,
       not really needed as checks on the timer counter are done, but better to
       be safe */
    vthis_cpu->sec_flags.fields.sched_timer_armed = 0;

    u8 vid = current->vid;

    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(vid, &vpartition_node);
    VBUG_ON(!vpartition);

    u32 terminated_count = 0;
    for (u32 i = 0; i < vpartition->vcpu_count; i++) {

        struct vcpu *vcpu = &vpartition->vcpus[i];
        struct vscheduler *vsched = vscheduler_of(vcpu);
        u32 vprocessor_id = vqueue_to_vprocessor_id(vcpu->vqueue_id);
        struct vper_cpu *target = vper_cpu_data(vprocessor_id);
        
        bool should_ipi_dequeue = false;
        bool should_dispatcher_dequeue = false;

        struct mcsnode vsched_node = INITIALIZE_MCSNODE();
        vmcs_lock_isr_save(&vsched->lock, &vsched_node);

        vcpu->vsched_metadata.terminate = true;

        if (vcpu != current) {

            vcpu_state_t state = vcpu->vsched_metadata.state;

            /* VRUNNING and VTRANSITIONING state should recover itself by 
               checking the terminate flag, VINITIALIZING should naturally
               destroy itself */

            switch (state) {

                case VRUNNING:
                    vipi_async(vprocessor_id);
                    break;

                case VREADY:
                    __vsched_dequeue(vcpu);

                    should_ipi_dequeue = true;
                    should_dispatcher_dequeue = true;
                    vcpu->vsched_metadata.state = VTERMINATED;
                    terminated_count++;
                    break;

                case VPAUSED:
                case VPV_SPINNING:
                    __vsched_dequeue_paused(vcpu);

                    should_dispatcher_dequeue = true;
                    vcpu->vsched_metadata.state = VTERMINATED;
                    terminated_count++;
                    break;

                default:
                    break;
            }

        } else {
            should_dispatcher_dequeue = true;
            current->vsched_metadata.state = VTERMINATED;
            terminated_count++;
        }

        vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);

        if (should_ipi_dequeue)
            vipi_ack_dequeue_all(vcpu);

        if (should_dispatcher_dequeue)
            vdispatcher_dequeue(target, vcpu);
    }

    vpartition_put(vid, &vpartition_node);

    u32 num_terminated = atomic32_fetch_and_add(&vpartition->num_terminated, 
                                                terminated_count);

    num_terminated += terminated_count;

    if (num_terminated == current->vpartition_num_cpus)
        vfinalize_teardown(vid);
    else
        vsched_recover();
}

void vfailure_recover_running(void)
{
    VBUG_ON(is_interrupts_enabled());

    struct vcpu *current = vcurrent_vcpu();
    struct vscheduler *vsched = vscheduler_of(current);

    struct mcsnode vsched_node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vsched->lock, &vsched_node);
    current->vsched_metadata.state = VTRANSITIONING;
    vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);

    vipi_ack();

#if TWANVISOR_VIPI_DRAIN_STRICT

    vipi_drain_no_yield();
    
#endif

    vfailure_recover();
}

int vteardown(u8 vid)
{
    struct mcsnode vpartition_node = INITIALIZE_MCSNODE();
    struct vpartition *vpartition = vpartition_get(vid, &vpartition_node);
    if (!vpartition)
        return -EINVAL;

    u32 terminated_count = 0;
    for (u32 i = 0; i < vpartition->vcpu_count; i++) {

        struct vcpu *vcpu = &vpartition->vcpus[i];
        struct vscheduler *vsched = vscheduler_of(vcpu);
        u32 vprocessor_id = vqueue_to_vprocessor_id(vcpu->vqueue_id);
        struct vper_cpu *target = vper_cpu_data(vprocessor_id);
        
        bool should_ipi_dequeue = false;
        bool should_dispatcher_dequeue = false;

        struct mcsnode vsched_node = INITIALIZE_MCSNODE();
        vmcs_lock_isr_save(&vsched->lock, &vsched_node);

        vcpu->vsched_metadata.terminate = true;
        vcpu_state_t state = vcpu->vsched_metadata.state;

        switch (state) {

            case VRUNNING:
                vipi_async(vprocessor_id);
                break;

            case VREADY:
                __vsched_dequeue(vcpu);

                should_ipi_dequeue = true;
                should_dispatcher_dequeue = true;
                vcpu->vsched_metadata.state = VTERMINATED;
                terminated_count++;
                break;

            case VPAUSED:
            case VPV_SPINNING:
                __vsched_dequeue_paused(vcpu);

                should_dispatcher_dequeue = true;
                vcpu->vsched_metadata.state = VTERMINATED;
                terminated_count++;
                break;

            default:
                break;
        }

        vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);

        if (should_ipi_dequeue)
            vipi_ack_dequeue_all(vcpu);

        if (should_dispatcher_dequeue)
            vdispatcher_dequeue(target, vcpu);
    }

    u32 num_vcpus = vpartition->vcpu_count;
    vpartition_put(vid, &vpartition_node);

    u32 num_terminated = atomic32_fetch_and_add(&vpartition->num_terminated, 
                                                terminated_count);
    num_terminated += terminated_count;

    if (num_terminated == num_vcpus)
        vdo_finalize_teardown(vid);
    
    return 0;
}

void vdo_transitioning_recover(struct vcpu *vcpu)
{
    u32 vprocessor_id = vqueue_to_vprocessor_id(vcpu->vqueue_id);

    struct vper_cpu *vtarget = vper_cpu_data(vprocessor_id);

    vdispatcher_dequeue(vtarget, vcpu);
    vipi_ack_dequeue_all(vcpu);

    struct vscheduler *vsched = vthis_vscheduler();
    struct mcsnode vsched_node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vsched->lock, &vsched_node);
    vcpu->vsched_metadata.state = VTERMINATED;
    vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);

    struct vpartition *vpartition = vcpu->vpartition;

    u32 num_terminated = atomic32_fetch_and_add(&vpartition->num_terminated, 1);
    num_terminated++;

    if (num_terminated == vcpu->vpartition_num_cpus)
        vdo_finalize_teardown(vcpu->vid);
}

void vtransitioning_recover_ipi(__unused u64 arg)
{
    struct interrupt_info *ctx = vthis_cpu_data()->vcpu_ctx;
    vdo_transitioning_recover(vcurrent_vcpu());
    vsched_get_spin(ctx);
}

void vtransitioning_recover(void)
{
    vemulate_self_ipi(vtransitioning_recover_ipi, 0);
}

#endif