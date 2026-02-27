#ifndef _SCHED_H_
#define _SCHED_H_

#include <kernel/sched/task.h>
#include <kernel/sched/sched_ctx.h>
#include <kernel/sched/sched_yield.h>
#include <kernel/sched/sched_timer.h>
#include <kernel/sched/sched_queue.h>
#include <subsys/sync/semaphore.h>
#include <subsys/sync/mcslock.h>

#if !SCHED_GLOBAL_QUEUE

bool try_steal_task(u32 criticality_level);

#endif

void sched_worker(__unused void *unused);

void sched_trampoline_ipi(__unused u64 unused);

void scheduler_init(void);

void sched_preempt(struct interrupt_info *ctx);

#endif