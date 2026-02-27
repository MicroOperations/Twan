#ifndef _SCHED_CTX_H_
#define _SCHED_CTX_H_

#include <kernel/sched/task.h>

void sched_save_current_ctx(struct task *current, struct interrupt_info *ctx);
void sched_put_current_ctx(struct interrupt_info *ctx, bool push);
void sched_set_ctx(struct interrupt_info *ctx, struct task *task);
void sched_switch_ctx(struct interrupt_info *ctx, struct task *task, bool push);

#endif