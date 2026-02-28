#ifndef _MCSLOCK_H_
#define _MCSLOCK_H_

#include <std.h>
#include <lib/atomic.h>

struct mcsnode
{
    atomic_ptr_t next;
    atomic32_t state;

#if CONFIG_TWANVISOR_PV_LOCKS
    atomic64_t processor_id;
#endif
};

struct mcslock
{
    atomic_ptr_t tail;
};

struct mcslock_isr
{
    struct mcslock lock;
    u64 flags;
};

#define MCS_LOCKED 0
#define MCS_UNLOCKED 1
#define MCS_PAUSED 2

#define INITIALIZE_MCSNODE()                    \
{                                               \
    .next = INITIALIZE_ATOMIC_PTR(NULL),        \
    .state = INITIALIZE_ATOMIC32(MCS_LOCKED) \
}

#define INITIALIZE_MCSLOCK()                \
{                                           \
    .tail = INITIALIZE_ATOMIC_PTR(NULL),    \
}

#define INITIALIZE_MCSLOCK_ISR()    \
{                                   \
    .lock = INITIALIZE_MCSLOCK(),   \
    .flags = 0                      \
}

void mcsnode_init(struct mcsnode *node);

void mcslock_init(struct mcslock *lock);

void __mcs_lock(struct mcslock *lock, struct mcsnode *node);
void __mcs_unlock(struct mcslock *lock, struct mcsnode *node);
bool __mcs_trylock(struct mcslock *lock, struct mcsnode *node);

void __pv_mcs_lock(struct mcslock *lock, struct mcsnode *node);
void __pv_mcs_unlock(struct mcslock *lock, struct mcsnode *node);
bool __pv_mcs_trylock(struct mcslock *lock, struct mcsnode *node);

void mcs_lock_disable_preemption(struct mcslock *lock, struct mcsnode *node);
void mcs_unlock_enable_preemption(struct mcslock *lock, struct mcsnode *node);
bool mcs_trylock_disable_preemption(struct mcslock *lock, struct mcsnode *node);

void mcs_lock(struct mcslock *lock, struct mcsnode *node);
void mcs_unlock(struct mcslock *lock, struct mcsnode *node);
bool mcs_trylock(struct mcslock *lock, struct mcsnode *node);

void mcslock_isr_init(struct mcslock_isr *lock);

void __mcs_lock_isr_save(struct mcslock *lock, struct mcsnode *node, 
                         u64 *flags);

void __mcs_unlock_isr_restore(struct mcslock *lock, struct mcsnode *node, 
                              u64 flags);

bool __mcs_trylock_isr_save(struct mcslock *lock, struct mcsnode *node, 
                            u64 *flags);

void __pv_mcs_lock_isr_save(struct mcslock *lock, struct mcsnode *node, 
                            u64 *flags);

void __pv_mcs_unlock_isr_restore(struct mcslock *lock, struct mcsnode *node,
                                 u64 flags);

bool __pv_mcs_trylock_isr_save(struct mcslock *lock, struct mcsnode *node, 
                                u64 *flags);

void mcs_lock_isr_save(struct mcslock_isr *lock, struct mcsnode *node);
void mcs_unlock_isr_restore(struct mcslock_isr *lock, struct mcsnode *node);
bool mcs_trylock_isr_save(struct mcslock_isr *lock, struct mcsnode *node);

#endif