#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <lib/x86_index.h>
#include <lib/atomic.h>

struct spinlock 
{
    atomic32_t next_ticket;
    atomic32_t current_ticket;
};

struct spinlock_isr 
{
    struct spinlock lock;
    u64 flags;
};

#define INITIALIZE_SPINLOCK()                   \
{                                               \
    .next_ticket = INITIALIZE_ATOMIC32(0),      \
    .current_ticket = INITIALIZE_ATOMIC32(0)    \
} 

#define INITIALIZE_SPINLOCK_ISR()               \
{                                               \
    .lock = INITIALIZE_SPINLOCK(),              \
    .flags = 0                                  \
}          

void spinlock_init(struct spinlock *lock);

void __spin_lock(struct spinlock *lock);
void __spin_unlock(struct spinlock *lock);
bool __spin_trylock(struct spinlock *lock);

void spin_lock_disable_preemption(struct spinlock *lock);
void spin_unlock_enable_preemption(struct spinlock *lock);
bool spin_trylock_disable_preemption(struct spinlock *lock);

void spin_lock(struct spinlock *lock);
void spin_unlock(struct spinlock *lock);
bool spin_trylock(struct spinlock *lock);

void spinlock_isr_init(struct spinlock_isr *lock);

void __spin_lock_isr_save(struct spinlock *lock, u64 *flags);
void __spin_unlock_isr_restore(struct spinlock *lock, u64 flags);
bool __spin_trylock_isr_save(struct spinlock *lock, u64 *flags);

void spin_lock_isr_save(struct spinlock_isr *lock);
void spin_unlock_isr_restore(struct spinlock_isr *lock);
bool spin_trylock_isr_save(struct spinlock_isr *lock);

#endif