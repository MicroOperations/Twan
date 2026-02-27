#ifndef _VSCHED_H_
#define _VSCHED_H_

#include <subsys/twanvisor/vsched/vcpu.h>
#include <subsys/twanvisor/vsched/vsched_ctx.h>
#include <subsys/twanvisor/vsched/vsched_dsa.h>

#define vsched_get_spin(ctx) spin_until(vsched_get((ctx)))

void __vsched_put(struct vcpu *vcpu, bool put_ctx, 
                  struct interrupt_info *ctx);

void __vsched_put_paused(struct vcpu *vcpu, bool pv_spin, bool put_ctx, 
                         struct interrupt_info *ctx);

void vsched_put(struct vcpu *vcpu, bool put_ctx, 
                struct interrupt_info *ctx);

bool vsched_put_paused(struct vcpu *vcpu, bool pv_spin, bool put_ctx, 
                       struct interrupt_info *ctx);

struct vcpu *__vsched_get(struct interrupt_info *ctx);
struct vcpu *vsched_get(struct interrupt_info *ctx);

void vthis_vsched_reschedule(struct vcpu *current, 
                             struct interrupt_info *ctx);

void vsched_preempt_isr(void);

#endif