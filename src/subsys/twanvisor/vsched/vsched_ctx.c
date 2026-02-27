#include <subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <subsys/twanvisor/vsched/vsched_ctx.h>
#include <subsys/twanvisor/twanvisor.h>

void vsched_put_ctx(struct vcpu *vcpu, struct interrupt_info *ctx)
{
    vcpu->context.cr8 = ctx->regs.cr8;
    vcpu->context.r15 = ctx->regs.r15;
    vcpu->context.r14 = ctx->regs.r14;
    vcpu->context.r13 = ctx->regs.r13;
    vcpu->context.r12 = ctx->regs.r12;
    vcpu->context.r11 = ctx->regs.r11;
    vcpu->context.r10 = ctx->regs.r10;
    vcpu->context.r9 = ctx->regs.r9;
    vcpu->context.r8 = ctx->regs.r8;
    vcpu->context.rsi = ctx->regs.rsi;
    vcpu->context.rdi = ctx->regs.rdi;
    vcpu->context.rdx = ctx->regs.rdx;
    vcpu->context.rcx = ctx->regs.rcx;
    vcpu->context.rbx = ctx->regs.rbx;
    vcpu->context.rax = ctx->regs.rax;
    vcpu->context.rbp = ctx->regs.rbp;
    vcpu->context.rsp = ctx->rsp;
    vcpu->context.rflags = ctx->rflags;
    vcpu->context.rip = ctx->rip;
    vcpu->context.fp_context = ctx->regs.fxsave_region;

    if (vcpu->put_callback_func)
        INDIRECT_BRANCH_SAFE(vcpu->put_callback_func());
}

void vsched_set_ctx(struct vcpu *vcpu, struct interrupt_info *ctx)
{
    ctx->regs.cr8 = vcpu->context.cr8;
    ctx->regs.r15 = vcpu->context.r15;
    ctx->regs.r14 = vcpu->context.r14;
    ctx->regs.r13 = vcpu->context.r13;
    ctx->regs.r12 = vcpu->context.r12;
    ctx->regs.r11 = vcpu->context.r11;
    ctx->regs.r10 = vcpu->context.r10;
    ctx->regs.r9 = vcpu->context.r9;
    ctx->regs.r8 = vcpu->context.r8;
    ctx->regs.rsi = vcpu->context.rsi;
    ctx->regs.rdi = vcpu->context.rdi;
    ctx->regs.rdx = vcpu->context.rdx;
    ctx->regs.rcx = vcpu->context.rcx;
    ctx->regs.rbx = vcpu->context.rbx;
    ctx->regs.rax = vcpu->context.rax;
    ctx->regs.rbp = vcpu->context.rbp;
    ctx->rsp = vcpu->context.rsp;
    ctx->rflags = vcpu->context.rflags;
    ctx->rip = vcpu->context.rip;
    ctx->regs.fxsave_region = vcpu->context.fp_context;

    if (vcpu->set_callback_func)
        INDIRECT_BRANCH_SAFE(vcpu->set_callback_func());
}

#endif