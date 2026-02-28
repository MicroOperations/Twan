#ifndef _WAITQ_H_
#define _WAITQ_H_

#include <kernel/sched/task.h>
#include <subsys/sync/mcslock.h>
#include <subsys/time/timeout.h>
#include <subsys/debug/kdbg/kdyn_assert.h>

struct waitq
{
    struct flat_priorityq flat_priorityq;
    struct mcslock_isr lock;
};

#define INITIALIZE_WAITQ()                          \
{                                                   \
    .flat_priorityq = INITIALIZE_FLAT_PRIORITYQ(),  \
    .lock = INITIALIZE_MCSLOCK_ISR()                \
}

/* 
    condition must satisfy these invariants

* must not grab any locks held by yield_wait_lock - deadlock
* have interrupts disabled - we are lazy waiting
*/

#define waitq_task_wait_until(waitq, insert_real, cond)                     \
do {                                                                        \
                                                                            \
    KDYNAMIC_ASSERT(!this_cpu_data()->handling_isr);                        \
                                                                            \
    u64 flags = read_flags_and_disable_interrupts();                        \
    while (1) {                                                             \
                                                                            \
        struct mcsnode waitq_node = INITIALIZE_MCSNODE();                   \
                                                                            \
        yield_wait_lock((waitq), &waitq_node);                              \
                                                                            \
        if ((cond)) {                                                       \
            yield_wait_unlock((waitq), &waitq_node);                        \
            break;                                                          \
        }                                                                   \
                                                                            \
        sched_yield_wait_and_unlock((waitq), (insert_real), &waitq_node,    \
                                    false, NULL, 0);                        \
    }                                                                       \
    write_flags(flags);                                                     \
} while (0)

#define wait_until_insert_real(waitq, cond) \
    waitq_task_wait_until((waitq), true, (cond))

#define wait_until(waitq, cond) \
    waitq_task_wait_until((waitq), false, (cond))

#if CONFIG_SUBSYS_TIMEOUT

#define waitq_task_wait_until_timeout(waitq, insert_real, cond, ticks)      \
({                                                                          \
    KDYNAMIC_ASSERT(!this_cpu_data()->handling_isr);                        \
                                                                            \
    u64 flags = read_flags_and_disable_interrupts();                        \
                                                                            \
    bool ret = false;                                                       \
    for (u32 i = 0; i < 2; i++) {                                           \
                                                                            \
        struct mcsnode timeout_node = INITIALIZE_MCSNODE();                 \
        struct mcsnode waitq_node = INITIALIZE_MCSNODE();                   \
                                                                            \
                                                                            \
        yield_wait_timeout_lock((waitq), &waitq_node, &timeout_node);       \
                                                                            \
        if ((cond)) {                                                       \
            yield_wait_timeout_unlock((waitq), &waitq_node, &timeout_node); \
            ret = true;                                                     \
            break;                                                          \
                                                                            \
        } else if (i == 1) {                                                \
                                                                            \
            yield_wait_timeout_unlock((waitq), &waitq_node, &waitq_node);   \
            break;                                                          \
        }                                                                   \
                                                                            \
        sched_yield_wait_and_unlock((waitq), (insert_real), &waitq_node,    \
                                    true, &timeout_node, (ticks));          \
                                                                            \
        if (timed_out()) {                                                  \
            clear_timed_out(current_task());                                \
            break;                                                          \
        }                                                                   \
    }                                                                       \
                                                                            \
    write_flags(flags);                                                     \
    ret;                                                                    \
})

#define wait_until_insert_real_timeout(waitq, cond, ticks) \
    waitq_task_wait_until_timeout((waitq), true, (cond), (ticks))   

#define wait_until_timeout(waitq, cond, ticks)  \
    waitq_task_wait_until_timeout((waitq), false, (cond), (ticks))

#endif

void waitq_init(struct waitq *waitq);
                      
void __waitq_insert(struct waitq *waitq, struct task *task, bool insert_real);

void __waitq_insert_timeout(struct waitq *waitq, struct task *task, 
                            bool insert_real, u32 ticks);

struct task *__waitq_popfront(struct waitq *waitq);
struct task *__waitq_wakeup_front(struct waitq *waitq);

void __waitq_dequeue_task(struct waitq *waitq, struct task *task);

#if CONFIG_SUBSYS_TIMEOUT

void waitq_do_insert(struct waitq *waitq, struct task *task, bool insert_real, 
                     bool timeout, u32 ticks);

#else

void waitq_do_insert(struct waitq *waitq, struct task *task, bool insert_real, 
                     __unused bool timeout, __unused u32 ticks);

#endif

void waitq_insert(struct waitq *waitq, struct task *task, bool insert_real);

void waitq_insert_timeout(struct waitq *waitq, struct task *task, 
                          bool insert_real, u32 ticks);

void waitq_wakeup_front(struct waitq *waitq, atomic_ptr_t *woken_task);

void waitq_flat_priorityq_dequeue_func(
    struct flat_priorityq_node *waiting_node);

void waitq_wakeup_all(struct waitq *waitq);

bool timeout_task_dequeue_from_waitq(struct task *task, bool timed_out);

#endif