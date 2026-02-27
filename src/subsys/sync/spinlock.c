#include <subsys/sync/spinlock.h>
#include <types.h>
#include <kernel/sched/sched.h>
#include <kernel/apic/apic.h>
#include <kernel/kapi.h>

/* TODO: pv spinlocks */

void spinlock_init(struct spinlock *lock)
{
    atomic32_set(&lock->next_ticket, 0);
    atomic32_set(&lock->current_ticket, 0);
}

void __spin_lock(struct spinlock *lock)
{
    int ticket = atomic32_fetch_and_add(&lock->next_ticket, 1);
    spin_until(atomic32_read(&lock->current_ticket) == ticket);
}

void __spin_unlock(struct spinlock *lock)
{
    atomic32_inc(&lock->current_ticket);
}

bool __spin_trylock(struct spinlock *lock)
{
    int current = atomic32_read(&lock->current_ticket);
    int expected = atomic32_read(&lock->next_ticket);

    if (current != expected)
        return false;

    return atomic32_cmpxchg(&lock->next_ticket, &expected, expected + 1);
}

void spin_lock_disable_preemption(struct spinlock *lock)
{
    current_task_disable_preemption();
    __spin_lock(lock);
}

void spin_unlock_enable_preemption(struct spinlock *lock)
{
    __spin_unlock(lock);
    current_task_enable_preemption();
}

bool spin_trylock_disable_preemption(struct spinlock *lock)
{
    current_task_disable_preemption();
    bool ret = __spin_trylock(lock);
    if (!ret)
        current_task_enable_preemption();

    return ret;
}

void spin_lock(struct spinlock *lock)
{
    spin_lock_disable_preemption(lock);
}

void spin_unlock(struct spinlock *lock)
{
    spin_unlock_enable_preemption(lock);
}

bool spin_trylock(struct spinlock *lock)
{
    return spin_trylock_disable_preemption(lock);
}

void spinlock_isr_init(struct spinlock_isr *lock)
{
    spinlock_init(&lock->lock);
    lock->flags = 0;
}

void __spin_lock_isr_save(struct spinlock *lock, u64 *flags)
{
    u64 __flags = read_flags_and_disable_interrupts();
    
    __spin_lock(lock);
    *flags = __flags;
}

void __spin_unlock_isr_restore(struct spinlock *lock, u64 flags)
{
    __spin_unlock(lock);
   write_flags(flags);
}

bool __spin_trylock_isr_save(struct spinlock *lock, u64 *flags)
{
    u64 __flags = read_flags_and_disable_interrupts();

    bool ret = __spin_trylock(lock);
    if (ret)
        *flags = __flags;
    else
        write_flags(__flags);

    return ret;
}

void spin_lock_isr_save(struct spinlock_isr *lock)
{
    __spin_lock_isr_save(&lock->lock, &lock->flags);
}

void spin_unlock_isr_restore(struct spinlock_isr *lock)
{
    __spin_unlock_isr_restore(&lock->lock, lock->flags);
}

bool spin_trylock_isr_save(struct spinlock_isr *lock)
{
    return __spin_trylock_isr_save(&lock->lock, &lock->flags);
}