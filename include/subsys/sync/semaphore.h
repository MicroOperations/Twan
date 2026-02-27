#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <subsys/sync/mcslock.h>
#include <kernel/sched/sched.h>
#include <kernel/sched/waitq.h>

/* count must be protected by waitq.lock */

struct semaphore 
{
    atomic64_t count;
    struct waitq waitq;
    struct mcslock lock;
};

#define INITIALIZE_SEMAPHORE(_count)            \
{                                               \
    .count = INITIALIZE_ATOMIC64((_count)),     \
    .waitq = INITIALIZE_WAITQ(),                \
    .lock = INITIALIZE_MCSLOCK()                \
}

void semaphore_init(struct semaphore *sem, u64 count);

bool semaphore_try_down(struct semaphore *sem);

void semaphore_down_insert_real(struct semaphore *sem);
void semaphore_down(struct semaphore *sem);

bool semaphore_down_insert_real_timeout(struct semaphore *sem, u32 ticks);
bool semaphore_down_timeout(struct semaphore *sem, u32 ticks);

void semaphore_up(struct semaphore *sem);

#endif