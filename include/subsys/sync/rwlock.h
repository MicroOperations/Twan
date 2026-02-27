#ifndef _RWLOCK_H_
#define _RWLOCK_H_

#include <subsys/sync/spinlock.h>

#define PF_LSB  0xFFFFFFF0
#define PF_READER_INC 0x100
#define PF_WRITER_BITS 0x3
#define PF_WRITER_PRESENT  0x2
#define PF_PHASE_ID  0x1

struct rwlock 
{
    atomic32_t rin;
    atomic32_t rout;
    atomic32_t win;
    atomic32_t wout;
};

struct rwlock_isr {
    struct rwlock lock;
    u64 flags;
};

#define INITIALIZE_RWLOCK()         \
{                                   \
    .rin = INITIALIZE_ATOMIC32(0),  \
    .rout = INITIALIZE_ATOMIC32(0), \
    .win = INITIALIZE_ATOMIC32(0),  \
    .wout = INITIALIZE_ATOMIC32(0), \
}

#define INITIALIZE_RWLOCK_ISR()     \
{                                   \
    .lock = INITIALIZE_RWLOCK(),    \
    .flags = 0                      \
}


void rwlock_init(struct rwlock *lock);

void __rw_lock_read(struct rwlock *lock);
void __rw_unlock_read(struct rwlock *lock);
void __rw_lock_write(struct rwlock *lock);
void __rw_unlock_write(struct rwlock *lock);

void rw_lock_read_disable_preemption(struct rwlock *lock);
void rw_unlock_read_enable_preemption(struct rwlock *lock);
void rw_lock_write_disable_preemption(struct rwlock *lock);
void rw_unlock_write_enable_preemption(struct rwlock *lock);

void rw_lock_read(struct rwlock *lock);
void rw_unlock_read(struct rwlock *lock);
void rw_lock_write(struct rwlock *lock);
void rw_unlock_write(struct rwlock *lock);

void rwlock_isr_init(struct rwlock_isr *lock);

void __rw_lock_read_isr_save(struct rwlock *lock, u64 *flags);
void __rw_unlock_read_isr_restore(struct rwlock *lock, u64 flags);
void __rw_lock_write_isr_save(struct rwlock *lock, u64 *flags);
void __rw_unlock_write_isr_restore(struct rwlock *lock, u64 flags);

void rw_lock_read_isr_save(struct rwlock_isr *lock);
void rw_unlock_read_isr_restore(struct rwlock_isr *lock);
void rw_lock_write_isr_save(struct rwlock_isr *lock);
void rw_unlock_write_isr_restore(struct rwlock_isr *lock);

#endif