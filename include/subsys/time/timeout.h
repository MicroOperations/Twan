#ifndef _TIMEOUT_H_
#define _TIMEOUT_H_

#include <subsys/sync/mcslock.h>
#include <kernel/sched/task.h>

typedef void (*timeout_lock_t)(struct mcsnode *node);
typedef void (*timeout_unlock_t)(struct mcsnode *node);
typedef void (*__timeout_insert_task_t)(struct task *task, u32 ticks);
typedef bool (*__timeout_dequeue_task_t)(struct task *task);
typedef u64 (*timeout_period_fs_t)(void);
typedef u64 (*timeout_frequency_hz_t)(void);

struct timeout_interface
{
    timeout_lock_t timeout_lock_func;
    timeout_unlock_t timeout_unlock_func;

    __timeout_insert_task_t __timeout_insert_task_func;
    __timeout_dequeue_task_t __timeout_dequeue_task_func;

    timeout_period_fs_t timeout_period_fs_func;
    timeout_frequency_hz_t timeout_frequency_hz_func;
};

struct timeout
{
    struct timeout_interface *interface;
    bool initialized;
};

int timeout_init(struct timeout_interface *interface);
bool is_timeout_initialized(void);

void timeout_lock(struct mcsnode *node);
void timeout_unlock(struct mcsnode *node);

void __timeout_insert_task(struct task *task, u32 ticks);
bool __timeout_dequeue_task(struct task *task);

u64 timeout_period_fs(void);
u64 timeout_frequency_hz(void);

void timeout_insert_task(struct task *task, u32 ticks);

#endif