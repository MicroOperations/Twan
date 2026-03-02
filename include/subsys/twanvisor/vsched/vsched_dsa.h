#ifndef _VSCHED_DSA_H_
#define _VSCHED_DSA_H_

#include <generated/autoconf.h>
#include <lib/atomic.h>
#include <lib/dsa/dq.h>
#include <subsys/twanvisor/vsched/vsched_mcs.h>
#include <subsys/twanvisor/vsched/vcpu.h>
#include <subsys/sync/mcslock.h>

struct vscheduler
{
#if CONFIG_TWANVISOR_VSCHED_MCQS
    struct dq queues[VSCHED_NUM_CRITICALITIES];
    struct dq paused_queue;
#endif

#if CONFIG_TWANVISOR_VSCHED_MCFS
    struct vcpu *frames[CONFIG_TWANVISOR_VSCHED_NUM_FRAMES];
#endif

    u8 vcriticality_level;
    struct mcslock_isr lock;
    atomic32_t kick;
    struct vcpu idle_vcpu;
};

STATIC_ASSERT(CONFIG_TWANVISOR_VSCHED_MCQS + CONFIG_TWANVISOR_VSCHED_MCFS == 1);

struct dq *__vsched_get_bucket(u8 *criticality);
void __vsched_push(struct vcpu *vcpu);
struct vcpu *__vsched_pop(void);
void __vsched_dequeue(struct vcpu *vcpu);

void __vsched_push_paused(struct vcpu *vcpu);
void __vsched_dequeue_paused(struct vcpu *vcpu);

#endif