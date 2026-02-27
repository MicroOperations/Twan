#include <kernel/sched/waitq.h>
#include <kernel/sched/task.h>
#include <kernel/kernel.h>
#include <subsys/time/timeout.h>
#include <subsys/debug/kdbg/kdbg.h>

/* waitq's are priority ordered and do not account for criticality levels for 
   simplicity and performance */

void waitq_init(struct waitq *waitq)
{
    memset(&waitq->flat_priorityq, 0, sizeof(struct dq));
    mcslock_isr_init(&waitq->lock);
}

void __waitq_insert(struct waitq *waitq, struct task *task, bool insert_real)
{
    flat_priorityq_insert(&waitq->flat_priorityq, &task->waiting_node,
                          insert_real ? task->metadata.real_priority :  
                          task->metadata.priority);
                
    task->metadata.waitq = waitq;
}

#if CONFIG_SUBSYS_TIMEOUT

void __waitq_insert_timeout(struct waitq *waitq, struct task *task, 
                            bool insert_real, u32 ticks)
{
    __waitq_insert(waitq, task, insert_real);
    __timeout_insert_task(task, ticks);
}

#endif

struct task *__waitq_popfront(struct waitq *waitq)
{
    struct flat_priorityq_node *waiting_node = 
        flat_priorityq_popfront(&waitq->flat_priorityq);

    if (!waiting_node)
        return NULL;

    struct task *waiting_task = waiting_node_to_task(waiting_node);
    waiting_task->metadata.waitq = NULL;

    return waiting_task;
}

struct task *__waitq_wakeup_front(struct waitq *waitq)
{
    struct task *waiting_task = __waitq_popfront(waitq);
    if (waiting_task) {

#if CONFIG_SUBSYS_TIMEOUT
        if (__timeout_dequeue_task(waiting_task))
            clear_timed_out(waiting_task);
#endif
        sched_push(waiting_task);
    }

    return waiting_task;
}

void __waitq_dequeue_task(struct waitq *waitq, struct task *task)
{
    flat_priorityq_dequeue(&waitq->flat_priorityq, &task->waiting_node);
    task->metadata.waitq = NULL;
}

#if CONFIG_SUBSYS_TIMEOUT

void waitq_do_insert(struct waitq *waitq, struct task *task, bool insert_real, 
                     bool timeout, u32 ticks)

#else

void waitq_do_insert(struct waitq *waitq, struct task *task, bool insert_real, 
                     __unused bool timeout, __unused u32 ticks)

#endif
{
    struct mcsnode waitq_node = INITIALIZE_MCSNODE();
    mcs_lock_isr_save(&waitq->lock, &waitq_node);

#if CONFIG_SUBSYS_TIMEOUT

    if (timeout)
        __waitq_insert_timeout(waitq, task, insert_real, ticks);
    else
        __waitq_insert(waitq, task, insert_real);

#else

    __waitq_insert(waitq, task, insert_real);

#endif

    mcs_unlock_isr_restore(&waitq->lock, &waitq_node);
}

void waitq_insert(struct waitq *waitq, struct task *task, bool insert_real)
{
    waitq_do_insert(waitq, task, insert_real, false, 0);
}

#if CONFIG_SUBSYS_TIMEOUT

void waitq_insert_timeout(struct waitq *waitq, struct task *task, 
                          bool insert_real, u32 ticks)
{
    struct mcsnode timeout_node = INITIALIZE_MCSNODE();

    timeout_lock(&timeout_node);
    waitq_do_insert(waitq, task, insert_real, true, ticks);
    timeout_unlock(&timeout_node);
}

#endif

void waitq_wakeup_front(struct waitq *waitq, atomic_ptr_t *woken_task)
{
#if CONFIG_SUBSYS_TIMEOUT

    struct mcsnode timeout_node = INITIALIZE_MCSNODE();
    struct mcsnode waitq_node = INITIALIZE_MCSNODE();

    timeout_lock(&timeout_node);
    mcs_lock_isr_save(&waitq->lock, &waitq_node);
    
    struct task *_woken_task = __waitq_wakeup_front(waitq);
    if (woken_task)
        atomic_ptr_set(woken_task, _woken_task);

    mcs_unlock_isr_restore(&waitq->lock, &waitq_node);
    timeout_unlock(&timeout_node);

#else 

    struct mcsnode waitq_node = INITIALIZE_MCSNODE();
    mcs_lock_isr_save(&waitq->lock, &waitq_node);
    
    struct task *_woken_task = __waitq_wakeup_front(waitq);
    if (woken_task)
        atomic_ptr_set(woken_task, _woken_task);

    mcs_unlock_isr_restore(&waitq->lock, &waitq_node);

#endif
}

void waitq_flat_priorityq_dequeue_func(struct flat_priorityq_node *waiting_node)
{
    struct task *task = waiting_node_to_task(waiting_node);
    if (!KBUG_ON(!task)) {

#if CONFIG_SUBSYS_TIMEOUT
        if (__timeout_dequeue_task(task))
            clear_timed_out(task);
#endif
        
        sched_push(task);
    }
}

void waitq_wakeup_all(struct waitq *waitq)
{
#if CONFIG_SUBSYS_TIMEOUT

    struct mcsnode timeout_node = INITIALIZE_MCSNODE();
    struct mcsnode waitq_node = INITIALIZE_MCSNODE();

    timeout_lock(&timeout_node);
    mcs_lock_isr_save(&waitq->lock, &waitq_node);

    flat_priorityq_dequeue_all(&waitq->flat_priorityq, 
                               waitq_flat_priorityq_dequeue_func);

    mcs_unlock_isr_restore(&waitq->lock, &waitq_node);
    timeout_unlock(&timeout_node);

#else 

    struct mcsnode waitq_node = INITIALIZE_MCSNODE();
    mcs_lock_isr_save(&waitq->lock, &waitq_node);

    flat_priorityq_dequeue_all(&waitq->flat_priorityq, 
                               waitq_flat_priorityq_dequeue_func);

    mcs_unlock_isr_restore(&waitq->lock, &waitq_node);

#endif
}

#if CONFIG_SUBSYS_TIMEOUT

bool timeout_task_dequeue_from_waitq(struct task *task, bool timed_out)
{
    struct waitq *waitq = task->metadata.waitq;

    struct mcsnode waitq_node = INITIALIZE_MCSNODE();
    mcs_lock_isr_save(&waitq->lock, &waitq_node);

    __waitq_dequeue_task(waitq, task);

    if (timed_out)
        set_timed_out(task);
    else 
        clear_timed_out(task);

    sched_push(task);

    mcs_unlock_isr_restore(&waitq->lock, &waitq_node);
    return true;
}

#endif