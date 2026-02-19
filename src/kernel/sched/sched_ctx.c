#include <include/kernel/sched/sched_ctx.h>
#include <include/kernel/sched/sched.h>
#include <include/kernel/kernel.h>

void sched_save_current_ctx(struct task *current, struct interrupt_info *ctx)
{
    current->context.cr8 = ctx->regs.cr8;
    current->context.r15 = ctx->regs.r15;
    current->context.r14 = ctx->regs.r14;
    current->context.r13 = ctx->regs.r13;
    current->context.r12 = ctx->regs.r12;
    current->context.r11 = ctx->regs.r11;
    current->context.r10 = ctx->regs.r10;
    current->context.r9 = ctx->regs.r9;
    current->context.r8 = ctx->regs.r8;
    current->context.rsi = ctx->regs.rsi;
    current->context.rdi = ctx->regs.rdi;
    current->context.rdx = ctx->regs.rdx;
    current->context.rcx = ctx->regs.rcx;
    current->context.rbx = ctx->regs.rbx;
    current->context.rax = ctx->regs.rax;
    current->context.rbp = ctx->regs.rbp;
    current->context.rsp = ctx->rsp;
    current->context.rflags = ctx->rflags;
    current->context.rip = ctx->rip;
    current->context.fp_context = ctx->regs.fxsave_region;
}

void sched_put_current_ctx(struct interrupt_info *ctx, bool push)
{
    struct task *current = current_task();
    sched_save_current_ctx(current, ctx);

    if (current->put_callback_func)
        INDIRECT_BRANCH_SAFE(current->put_callback_func());

    set_current_task(NULL);
    if (push)
        sched_push(current);
}

void sched_set_ctx(struct interrupt_info *ctx, struct task *task)
{
    ctx->regs.cr8 = task->context.cr8;
    ctx->regs.r15 = task->context.r15;
    ctx->regs.r14 = task->context.r14;
    ctx->regs.r13 = task->context.r13;
    ctx->regs.r12 = task->context.r12;
    ctx->regs.r11 = task->context.r11;
    ctx->regs.r10 = task->context.r10;
    ctx->regs.r9 = task->context.r9;
    ctx->regs.r8 = task->context.r8;
    ctx->regs.rsi = task->context.rsi;
    ctx->regs.rdi = task->context.rdi;
    ctx->regs.rdx = task->context.rdx;
    ctx->regs.rcx = task->context.rcx;
    ctx->regs.rbx = task->context.rbx;
    ctx->regs.rax = task->context.rax;
    ctx->regs.rbp = task->context.rbp;
    ctx->rsp = task->context.rsp;
    ctx->rflags = task->context.rflags;
    ctx->rip = task->context.rip;
    ctx->regs.fxsave_region = task->context.fp_context;

    set_current_task(task);

    if (task->set_callback_func)
        INDIRECT_BRANCH_SAFE(task->set_callback_func());
}

void sched_switch_ctx(struct interrupt_info *ctx, struct task *task, bool push)
{
    sched_put_current_ctx(ctx, push);
    sched_set_ctx(ctx, task);
}