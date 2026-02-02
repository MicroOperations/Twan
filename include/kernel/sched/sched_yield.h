#ifndef _SCHED_YIELD_H_
#define _SCHED_YIELD_H_

#include <include/kernel/sched/task.h>
#include <include/subsys/sync/mcslock.h>

struct yield_wait_arg
{
    struct waitq *waitq;
    bool locked;
    
    bool insert_real;

    struct mcsnode *waitq_node;

    bool timeout;
    struct mcsnode *timeout_node;
    u64 ticks;
};

#define yield_wait_lock(waitq, waitq_node) \
    mcs_lock_isr_save(&(waitq)->lock, (waitq_node))

#define yield_wait_unlock(waitq, waitq_node) \
    mcs_unlock_isr_restore(&(waitq)->lock, (waitq_node))

#define yield_wait_timeout_lock(waitq, waitq_node, timeout_node)            \
do {                                                                        \
    timeout_lock((timeout_node));                                           \
    yield_wait_lock((waitq), (waitq_node));                                 \
} while (0)

#define yield_wait_timeout_unlock(waitq, waitq_node, timeout_node)          \
do {                                                                        \
    yield_wait_unlock((waitq), (waitq_node));                               \
    timeout_unlock((timeout_node));                                         \
} while (0)

bool sched_try_answer_yield_request(void);
bool sched_should_request_yield(struct task *task);

void sched_yield_ipi(__unused u64 unused);
void sched_yield(void);

void sched_yield_wait_ipi(u64 _arg);

void sched_yield_wait_and_unlock(struct waitq *waitq, bool insert_real, 
                                 struct mcsnode *waitq_node, bool timeout, 
                                 struct mcsnode *timeout_node, u64 ticks);

#endif