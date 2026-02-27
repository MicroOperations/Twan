#include <subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <subsys/twanvisor/vsched/vsched_dsa.h>
#include <subsys/twanvisor/vsched/vsched_mcs.h>
#include <subsys/twanvisor/twanvisor.h>

struct dq *__vsched_get_bucket(u8 *criticality)
{
    u8 criticality_level = __vsched_mcs_read_criticality_level_local();
    struct vscheduler *vsched = vthis_vscheduler();

    struct dq *queue = &vsched->queues[criticality_level];

    if (dq_peekfront(queue)) {

        *criticality = criticality_level;
        return queue;

    } else if (criticality_level == VSCHED_MIN_CRITICALITY) {
        return NULL;
    }

    queue = &vsched->queues[VSCHED_MIN_CRITICALITY];
    if (!dq_peekfront(queue))
        return NULL;

    *criticality = VSCHED_MIN_CRITICALITY;
    return queue;
}

void __vsched_push(struct vcpu *vcpu)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);
    u8 criticality = vcpu->vsched_metadata.criticality;

    for (u32 i = 0; i <= criticality; i++)
        dq_pushback(&vsched->queues[i], &vcpu->vsched_nodes[i]);
}

struct vcpu *__vsched_pop(void)
{
    u8 criticality;
    struct dq *dq = __vsched_get_bucket(&criticality);
    if (!dq)
        return NULL;

    struct list_double *node = dq_popfront(dq);
    return vsched_node_to_vcpu(node, criticality);
}

void __vsched_dequeue(struct vcpu *vcpu)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);
    u8 criticality = vcpu->vsched_metadata.criticality;

    for (u32 i = 0; i <= criticality; i++) {

        struct dq *dq = &vsched->queues[i];
        struct list_double *node = &vcpu->vsched_nodes[i];

        if (dq_is_queued(dq, node))
            dq_dequeue(dq, node);
    }
}

void __vsched_push_paused(struct vcpu *vcpu)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);
    dq_pushback(&vsched->paused_queue, &vcpu->vsched_nodes[0]);
}

void __vsched_dequeue_paused(struct vcpu *vcpu)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);
    if (dq_is_queued(&vsched->paused_queue, &vcpu->vsched_nodes[0]))
        dq_dequeue(&vsched->paused_queue, &vcpu->vsched_nodes[0]);   
}

#endif