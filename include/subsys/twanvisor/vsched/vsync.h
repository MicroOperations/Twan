#ifndef _VSYNC_H_
#define _VSYNC_H_

#include <subsys/sync/mcslock.h>
#include <subsys/sync/rwlock.h>

void vmcs_lock_isr_save(struct mcslock_isr *lock, struct mcsnode *node);
void vmcs_unlock_isr_restore(struct mcslock_isr *lock, struct mcsnode *node);

void vrw_lock_read_isr_save(struct rwlock_isr *lock);
void vrw_unlock_read_isr_restore(struct rwlock_isr *lock); 
void vrw_lock_write_isr_save(struct rwlock_isr *lock);
void vrw_unlock_write_isr_restore(struct rwlock_isr *lock);

#endif