#ifndef _VSCHED_CTX_H_
#define _VSCHED_CTX_H_

#include <subsys/twanvisor/vsched/vcpu.h>
#include <subsys/twanvisor/varch.h>

void vsched_put_ctx(struct vcpu *vcpu, struct interrupt_info *ctx);
void vsched_set_ctx(struct vcpu *vcpu, struct interrupt_info *ctx);
void vsched_enter_ctx(struct vcpu *vcpu, struct interrupt_info *ctx);

#endif