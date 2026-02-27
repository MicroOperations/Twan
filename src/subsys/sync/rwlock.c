#include <subsys/sync/rwlock.h>
#include <kernel/kernel.h>
#include <kernel/kapi.h>

/* TODO: pv rwlocks */

void rwlock_init(struct rwlock *lock)
{
   atomic32_set(&lock->rin, 0);
   atomic32_set(&lock->rout, 0);
   atomic32_set(&lock->win, 0);
   atomic32_set(&lock->wout, 0);
}

void __rw_lock_read(struct rwlock *lock)
{
    u32 wp = atomic32_fetch_and_add(&lock->rin, PF_READER_INC) & PF_WRITER_BITS;
    if (wp == 0)
        return;

    spin_until((atomic32_read(&lock->rin) & PF_WRITER_BITS) != wp);
}

void __rw_unlock_read(struct rwlock *lock)
{
    atomic32_add(&lock->rout, PF_READER_INC);
}

void __rw_lock_write(struct rwlock *lock)
{
    int ticket = atomic32_fetch_and_add(&lock->win, 1);
    spin_until(ticket == atomic32_read(&lock->wout));

    ticket = atomic32_fetch_and_add(&lock->rin,
                                    (ticket & PF_PHASE_ID) | PF_WRITER_PRESENT);

    spin_until(ticket == atomic32_read(&lock->rout));
}

void __rw_unlock_write(struct rwlock *lock)
{
    atomic32_fetch_and_and(&lock->rin, PF_LSB);
    atomic32_inc(&lock->wout);
}

void rw_lock_read_disable_preemption(struct rwlock *lock)
{
    current_task_disable_preemption();
    __rw_lock_read(lock);
}

void rw_unlock_read_enable_preemption(struct rwlock *lock)
{
    __rw_unlock_read(lock);
    current_task_enable_preemption();
}

void rw_lock_write_disable_preemption(struct rwlock *lock)
{
    current_task_disable_preemption();
    __rw_lock_write(lock);
}

void rw_unlock_write_enable_preemption(struct rwlock *lock)
{
    __rw_unlock_write(lock);
    current_task_enable_preemption();
}

void rw_lock_read(struct rwlock *lock)
{
    rw_lock_read_disable_preemption(lock);
}

void rw_unlock_read(struct rwlock *lock)
{
    rw_unlock_read_enable_preemption(lock);
}

void rw_lock_write(struct rwlock *lock)
{
    rw_lock_write_disable_preemption(lock);
}

void rw_unlock_write(struct rwlock *lock)
{
    rw_unlock_write_enable_preemption(lock);
}

void rwlock_isr_init(struct rwlock_isr *lock)
{
    rwlock_init(&lock->lock);
    lock->flags = 0;
}

void __rw_lock_read_isr_save(struct rwlock *lock, u64 *flags)
{
    u64 __flags = read_flags_and_disable_interrupts();

    __rw_lock_read(lock);
    *flags = __flags;
}

void __rw_unlock_read_isr_restore(struct rwlock *lock, u64 flags)
{
    __rw_unlock_read(lock);
    write_flags(flags);
}

void __rw_lock_write_isr_save(struct rwlock *lock, u64 *flags)
{
    u64 __flags = read_flags_and_disable_interrupts();

    __rw_lock_write(lock);
    *flags = __flags;
}

void __rw_unlock_write_isr_restore(struct rwlock *lock, u64 flags)
{
    __rw_unlock_write(lock);
    write_flags(flags);
}

void rw_lock_read_isr_save(struct rwlock_isr *lock)
{
    __rw_lock_read_isr_save(&lock->lock, &lock->flags);
}

void rw_unlock_read_isr_restore(struct rwlock_isr *lock)
{
    __rw_unlock_read_isr_restore(&lock->lock, lock->flags);
}

void rw_lock_write_isr_save(struct rwlock_isr *lock)
{
    __rw_lock_write_isr_save(&lock->lock, &lock->flags);
}

void rw_unlock_write_isr_restore(struct rwlock_isr *lock)
{
    __rw_unlock_write_isr_restore(&lock->lock, lock->flags);
}