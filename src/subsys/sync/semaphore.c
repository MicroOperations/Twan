#include <subsys/sync/semaphore.h>
#if CONFIG_SUBSYS_SEMAPHORE

#include <kernel/kapi.h>
#include <kernel/sched/sched.h>
#include <subsys/debug/kdbg/kdbg.h>

void semaphore_init(struct semaphore *sem, u64 count)
{
    atomic64_set(&sem->count, count);
    waitq_init(&sem->waitq);
    mcslock_init(&sem->lock);
}

bool semaphore_try_down(struct semaphore *sem)
{
    /* the lock here is solely for arbitration logic, going lockless here would
       increase the difficulty to prove that this routine would have a strict 
       bounded wcet, as we would have to spin until we successfully check the 
       count and decrement it without interference, if another core repeatedly
       calls semaphore_try_down and we dont have any hardware level arbitration
       logic, it would be harder to prove that this always terminates */

    struct mcsnode node = INITIALIZE_MCSNODE();
    mcs_lock(&sem->lock, &node);

    bool ret = (u64)atomic64_read(&sem->count) > 0;
    if (ret)
        atomic64_dec(&sem->count);
    
    mcs_unlock(&sem->lock, &node);
    return ret;
}

void semaphore_down_insert_real(struct semaphore *sem)
{
    wait_until_insert_real(&sem->waitq, semaphore_try_down(sem));
}

void semaphore_down(struct semaphore *sem)
{
    wait_until(&sem->waitq, semaphore_try_down(sem));
}

#if CONFIG_SUBSYS_TIMEOUT

bool semaphore_down_insert_real_timeout(struct semaphore *sem, u32 ticks)
{
    return wait_until_insert_real_timeout(&sem->waitq, semaphore_try_down(sem), 
                                          ticks);
}

bool semaphore_down_timeout(struct semaphore *sem, u32 ticks)
{
    return wait_until_timeout(&sem->waitq, semaphore_try_down(sem), ticks);   
}

#endif

void semaphore_up(struct semaphore *sem)
{
    u64 count = atomic64_read(&sem->count);
    KBUG_ON(count == UINT64_MAX);

    atomic64_inc(&sem->count);
    waitq_wakeup_front(&sem->waitq, NULL);
}

#endif