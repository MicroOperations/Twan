#ifndef _VSCHED_DSA_H_
#define _VSCHED_DSA_H_

#include <lib/atomic.h>
#include <lib/dsa/dq.h>
#include <subsys/twanvisor/vsched/vsched_mcs.h>
#include <subsys/twanvisor/vsched/vcpu.h>
#include <subsys/sync/mcslock.h>

struct vscheduler
{
    u8 vcriticality_level;
    struct dq queues[VSCHED_NUM_CRITICALITIES];
    struct dq paused_queue;
    struct mcslock_isr lock;
    atomic32_t kick;
    struct vcpu idle_vcpu;
};

struct dq *__vsched_get_bucket(u8 *criticality);
void __vsched_push(struct vcpu *vcpu);
struct vcpu *__vsched_pop(void);
void __vsched_dequeue(struct vcpu *vcpu);

void __vsched_push_paused(struct vcpu *vcpu);
void __vsched_dequeue_paused(struct vcpu *vcpu);

#endif