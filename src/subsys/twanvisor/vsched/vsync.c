#include <subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <subsys/twanvisor/vsched/vsync.h>

void vmcs_lock_isr_save(struct mcslock_isr *lock, struct mcsnode *node)
{
    __mcs_lock_isr_save(&lock->lock, node, &lock->flags);
}

void vmcs_unlock_isr_restore(struct mcslock_isr *lock, struct mcsnode *node)
{
    __mcs_unlock_isr_restore(&lock->lock, node, lock->flags);
}

bool vmcs_trylock_isr_save(struct mcslock_isr *lock, struct mcsnode *node)
{
    return __mcs_trylock_isr_save(&lock->lock, node, &lock->flags);
}

void vrw_lock_read_isr_save(struct rwlock_isr *lock)
{
    __rw_lock_read_isr_save(&lock->lock, &lock->flags);
}

void vrw_unlock_read_isr_restore(struct rwlock_isr *lock)
{
    __rw_unlock_read_isr_restore(&lock->lock, lock->flags);
}

void vrw_lock_write_isr_save(struct rwlock_isr *lock)
{
    __rw_lock_write_isr_save(&lock->lock, &lock->flags);
}

void vrw_unlock_write_isr_restore(struct rwlock_isr *lock)
{
    __rw_unlock_write_isr_restore(&lock->lock, lock->flags);
}

#endif