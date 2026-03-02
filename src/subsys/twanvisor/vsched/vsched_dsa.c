#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vsched/vsched_dsa.h>
#include <subsys/twanvisor/vsched/vsched_mcs.h>
#include <subsys/twanvisor/twanvisor.h>

#if CONFIG_TWANVISOR_VSCHED_MCQS

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

    atomic32_set(&vsched->kick, VSCHED_IDLE_KICK_SET);
}

void __vsched_push_paused(struct vcpu *vcpu)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);
    dq_pushback(&vsched->paused_queue, &vcpu->vsched_nodes[0]);
}

bool __vsched_is_current_preemptible(struct vcpu *current)
{
    u8 criticality_level = __vsched_mcs_read_criticality_level_local();
    u8 criticality = current->vsched_metadata.criticality;

    u8 next_criticality;
    return __vsched_get_bucket(&next_criticality) && 
                (criticality < criticality_level || 
                 next_criticality >= criticality_level);
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

void __vsched_dequeue_paused(struct vcpu *vcpu)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);
    if (dq_is_queued(&vsched->paused_queue, &vcpu->vsched_nodes[0]))
        dq_dequeue(&vsched->paused_queue, &vcpu->vsched_nodes[0]);   
}

void __vsched_unpause(struct vcpu *vcpu)
{
    __vsched_dequeue_paused(vcpu);
    vcpu->vsched_metadata.state = VREADY;
    __vsched_push(vcpu);
}

void __vsched_update_criticality(struct vcpu *vcpu, u8 criticality)
{
    if (vcpu->vsched_metadata.state == VREADY) {

        __vsched_dequeue(vcpu);
        vcpu->vsched_metadata.criticality = criticality;
        __vsched_push(vcpu);

    } else {
        vcpu->vsched_metadata.criticality = criticality;
    }
}

#endif


#if CONFIG_TWANVISOR_VSCHED_MCFS

bool __vsched_is_current_preemptible(struct vcpu *current)
{
    struct vscheduler *vsched = vthis_vscheduler();
    u8 criticality_level = __vsched_mcs_read_criticality_level_local();
    u8 criticality = current->vsched_metadata.criticality;
    u32 clock = vsched->clock;

    for (u32 i = 0; i < (ARRAY_LEN(vsched->frames) - 1); i++) {

        u32 cur_idx = (clock + i) % ARRAY_LEN(vsched->frames);
        struct vcpu *vcpu = vsched->frames[cur_idx];

        if (vcpu && vcpu->vsched_metadata.state == VREADY) {

            u8 next_criticality = vcpu->vsched_metadata.criticality;

            if (criticality < criticality_level || 
                next_criticality >= criticality_level) {
                return true;
            }
        }
    }

    return false;
}

struct vcpu *__vsched_pop(void)
{
    struct vscheduler *vsched = vthis_vscheduler();
    u8 criticality_level = __vsched_mcs_read_criticality_level_local();
    u32 clock = vsched->clock;

    u32 first_idx = 0;
    struct vcpu *first_vcpu = NULL;
    for (u32 i = 0; i < ARRAY_LEN(vsched->frames); i++) {

        u32 cur_idx = (clock + i) % ARRAY_LEN(vsched->frames);
        struct vcpu *vcpu = vsched->frames[cur_idx];

        if (vcpu && vcpu->vsched_metadata.state == VREADY) {

            if (!first_vcpu) {
                first_idx = cur_idx;
                first_vcpu = vcpu;
            }

            if (vcpu->vsched_metadata.criticality >= criticality_level) {
                vsched->clock = cur_idx + 1;
                return vcpu;
            }
        }
    }

    if (first_vcpu)
        vsched->clock = first_idx + 1;

    return first_vcpu;
}

void __vsched_dequeue(struct vcpu *vcpu)
{
    struct vscheduler *vsched = vscheduler_of(vcpu);

    for (u32 i = 0; i < ARRAY_LEN(vsched->frames); i++) {

        if (vsched->frames[i] == vcpu)
            vsched->frames[i] = NULL;
    }
}

void __vsched_dequeue_paused(struct vcpu *vcpu)
{
    __vsched_dequeue(vcpu);
}

void __vsched_unpause(struct vcpu *vcpu)
{
    vcpu->vsched_metadata.state = VREADY;
}

void __vsched_update_criticality(struct vcpu *vcpu, u8 criticality)
{
    vcpu->vsched_metadata.criticality = criticality;
}

#endif

#endif