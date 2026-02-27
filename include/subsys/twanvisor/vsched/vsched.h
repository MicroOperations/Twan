#ifndef _VSCHED_H_
#define _VSCHED_H_

#include <subsys/twanvisor/vsched/vcpu.h>
#include <subsys/twanvisor/vsched/vsched_ctx.h>
#include <subsys/twanvisor/vsched/vsched_dsa.h>

#define VSCHED_IDLE_KICK_UNSET 0
#define VSCHED_IDLE_KICK_SET 1

#define vsched_get_spin(ctx) vsched_get_or_idle((ctx))

void __vsched_put(struct vcpu *vcpu, bool put_ctx, 
                  struct interrupt_info *ctx);

void __vsched_put_paused(struct vcpu *vcpu, bool pv_spin, bool put_ctx, 
                         struct interrupt_info *ctx);

void vsched_put(struct vcpu *vcpu, bool put_ctx, 
                struct interrupt_info *ctx);

bool vsched_put_paused(struct vcpu *vcpu, bool pv_spin, bool put_ctx, 
                       struct interrupt_info *ctx);

struct vcpu *__vsched_get(struct interrupt_info *ctx);
struct vcpu *vsched_get_or_idle(struct interrupt_info *ctx);

void vthis_vsched_reschedule(struct vcpu *current, 
                             struct interrupt_info *ctx);

void vsched_preempt_isr(void);
void vidle_vcpu_loop(void);

#endif