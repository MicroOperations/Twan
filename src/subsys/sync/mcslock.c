#include <subsys/sync/mcslock.h>
#include <kernel/kapi.h>
#include <lib/libtwanvisor/libvcalls.h>
#include <lib/libtwanvisor/libvc.h>

void mcsnode_init(struct mcsnode *node)
{
    atomic_ptr_set(&node->next, NULL);
    atomic32_set(&node->state, MCS_LOCKED);
}

void mcslock_init(struct mcslock *lock)
{
    atomic_ptr_set(&lock->tail, NULL);
}

void __mcs_lock(struct mcslock *lock, struct mcsnode *node)
{
    struct mcsnode *prev = atomic_ptr_xchg(&lock->tail, node);

    if (prev) {
        atomic_ptr_set(&prev->next, node);
        spin_until(atomic32_read(&node->state) == MCS_UNLOCKED);
    }
}

void __mcs_unlock(struct mcslock *lock, struct mcsnode *node)
{
    struct mcsnode *next = atomic_ptr_read(&node->next);

    if (!next) {

        void *expected = node;
        if (atomic_ptr_cmpxchg(&lock->tail, &expected, NULL))
            return;

        spin_until(next = atomic_ptr_read(&node->next));
    }

    atomic32_set(&next->state, MCS_UNLOCKED);
}

bool __mcs_trylock(struct mcslock *lock, struct mcsnode *node)
{
   return atomic_ptr_cmpxchg(&lock->tail, NULL, node);
}

void __pv_mcs_lock(struct mcslock *lock, struct mcsnode *node)
{
#if TWANVISOR_PV_LOCKS

    if (twan()->flags.fields.twanvisor_on != 0) {

        atomic64_set(&node->processor_id, this_processor_id());

        struct mcsnode *prev = atomic_ptr_xchg(&lock->tail, node);

        if (prev) {

            long long processor_id = atomic64_read(&prev->processor_id);
            atomic_ptr_set(&prev->next, node);

            u32 iterations = 0;
            while (atomic32_read(&node->state) != MCS_UNLOCKED) {

                if (processor_id < 0) {
                    cpu_relax();
                    continue;
                }

                if (iterations != CONFIG_TWANVISOR_PV_LOCK_THRESHOLD) {
                    
                    iterations++;
                    cpu_relax();
                    continue;
                }

                iterations = 0;
                if (!tv_vis_vcpu_active(processor_id)) {

                    int expected = MCS_LOCKED;
                    if (atomic32_cmpxchg(&node->state, &expected, MCS_PAUSED))
                        tv_vpv_spin_pause();
                    
                } else {
                    cpu_relax();
                }
            }
        }

        return;
    }

    atomic64_set(&node->processor_id, -1);
    
#endif

    __mcs_lock(lock, node);
}

void __pv_mcs_unlock(struct mcslock *lock, struct mcsnode *node)
{
#if TWANVISOR_PV_LOCKS

    if (twan()->flags.fields.twanvisor_on != 0) {
        
        struct mcsnode *next = atomic_ptr_read(&node->next);

        if (!next) {

            void *expected = node;
            if (atomic_ptr_cmpxchg(&lock->tail, &expected, NULL))
                return;

            spin_until(next = atomic_ptr_read(&node->next));
        }

        int expected2 = MCS_LOCKED;
        if (!atomic32_cmpxchg(&next->state, &expected2, MCS_UNLOCKED)) {

            long long processor_id = atomic64_read(&next->processor_id);
            atomic32_set(&next->state, MCS_UNLOCKED);

            tv_vpv_spin_kick(processor_id);
        }

        return;
    }

#endif

    __mcs_unlock(lock, node);
}

bool __pv_mcs_trylock(struct mcslock *lock, struct mcsnode *node)
{
#if TWANVISOR_PV_LOCKS

    if (twan()->flags.fields.twanvisor_on != 0)
        atomic64_set(&node->processor_id, this_processor_id());
    else
        atomic64_set(&node->processor_id, -1);

#endif

    return __mcs_trylock(lock, node);
}

void mcs_lock_disable_preemption(struct mcslock *lock, struct mcsnode *node)
{
    current_task_disable_preemption();
    __pv_mcs_lock(lock, node);
}

void mcs_unlock_enable_preemption(struct mcslock *lock, struct mcsnode *node)
{
    __pv_mcs_unlock(lock, node);
    current_task_enable_preemption();
}

bool mcs_trylock_disable_preemption(struct mcslock *lock, struct mcsnode *node)
{
    current_task_disable_preemption();
    bool ret = __pv_mcs_trylock(lock, node);
    if (!ret)
        current_task_enable_preemption();

    return ret;
}

void mcs_lock(struct mcslock *lock, struct mcsnode *node)
{
    mcs_lock_disable_preemption(lock, node);
}

void mcs_unlock(struct mcslock *lock, struct mcsnode *node)
{
    mcs_unlock_enable_preemption(lock, node);
}

bool mcs_trylock(struct mcslock *lock, struct mcsnode *node)
{
    return mcs_trylock_disable_preemption(lock, node);
}

void mcslock_isr_init(struct mcslock_isr *lock)
{
    mcslock_init(&lock->lock);
    lock->flags = 0;
}

void __mcs_lock_isr_save(struct mcslock *lock, struct mcsnode *node, u64 *flags)
{
    u64 __flags = read_flags_and_disable_interrupts();
    
    __mcs_lock(lock, node);
    *flags = __flags;
}

void __mcs_unlock_isr_restore(struct mcslock *lock, struct mcsnode *node, 
                              u64 flags)
{
    __mcs_unlock(lock, node);
   write_flags(flags);
}

bool __mcs_trylock_isr_save(struct mcslock *lock, struct mcsnode *node, 
                            u64 *flags)
{
    u64 __flags = read_flags_and_disable_interrupts();

    bool ret = __mcs_trylock(lock, node);
    if (ret)
        *flags = __flags;
    else
        write_flags(__flags);

    return ret;
}

void __pv_mcs_lock_isr_save(struct mcslock *lock, struct mcsnode *node, u64 *flags)
{
    u64 __flags = read_flags_and_disable_interrupts();
    
    __pv_mcs_lock(lock, node);
    *flags = __flags;
}

void __pv_mcs_unlock_isr_restore(struct mcslock *lock, struct mcsnode *node, 
                                 u64 flags)
{
    __pv_mcs_unlock(lock, node);
   write_flags(flags);
}

bool __pv_mcs_trylock_isr_save(struct mcslock *lock, struct mcsnode *node, 
                               u64 *flags)
{
    u64 __flags = read_flags_and_disable_interrupts();

    bool ret = __pv_mcs_trylock(lock, node);
    if (ret)
        *flags = __flags;
    else
        write_flags(__flags);

    return ret;
}

void mcs_lock_isr_save(struct mcslock_isr *lock, struct mcsnode *node)
{
    __pv_mcs_lock_isr_save(&lock->lock, node, &lock->flags);
}

void mcs_unlock_isr_restore(struct mcslock_isr *lock, struct mcsnode *node)
{
    __pv_mcs_unlock_isr_restore(&lock->lock, node, lock->flags);
}

bool mcs_trylock_isr_save(struct mcslock_isr *lock, struct mcsnode *node)
{
    return __pv_mcs_trylock_isr_save(&lock->lock, node, &lock->flags);
}