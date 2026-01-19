# Mcslock

mcs locks are used frequently throughout Twan for synchronisation, mcs locks should be used for short, critical sections, as they busy wait. mcs locks in Twan come in two forms, mcslock, and mcslock_isr. mcslock does not disable interrupts during critical sections, however it does disable preemption once a critical section starts, and reenables preemption once the critical section is over. If the scheduler timer fires during critical section, it'll set a flag associated with the task so that it hands control back to the scheduler once it is in a state where it is preemptible again. mcslock_isr on the other hand disables interrupts once a critical section starts, and reenables it once it ends. the decision made to reenable preemption and interrupts is based on whether or not they were enabled before the critical section was entered, if they were enabled, then they will be reenabled.

## Configuration

---
```
TWANVISOR_PV_LOCKS
```

Enables paravirtualised mcs locks, when this is enabled, vcpus within TwanRTOS will block if their predecessor in the mcs locks waiting queue is not active, and will be unblocked once the lock has been handed to them. This is designed to battle the lock holder/waiter preemption problem, by being aware of vcpu scheduling states.

## API's

```<include/subsys/sync/mcslock.h>```

---
```
INITIALIZE_MCSNODE()
```

Macro to initialize an mcs node

---
```
INITIALIZE_MCSLOCK()
```

Macro to initialize an mcslock

---
```
INITIALIZE_MCSLOCK_ISR()
```

Macro to initialize an mcslock_isr

---
```
void mcsnode_init(struct mcsnode *node)
```

Routine to initialize an mcsnode at runtime

---
```
void mcslock_init(struct mcslock *lock)
```

Routine to initialize an mcslock at runtime

---
```
void mcs_lock(struct mcslock *lock, struct mcsnode *node)
```

Disables preemption and locks an mcslock, busy waits in the contended case

---
```
void mcs_unlock(struct mcslock *lock, struct mcsnode *node)
```

Unlocks an mcslock, Reenables preemption if it was enabled before the critical section started

---
```
bool mcs_trylock(struct mcslock *lock, struct mcsnode *node)
```

Routine which attempts to acquire an mcs lock, returns true on acquisition otherwise false

---
```
void mcslock_isr_init(struct mcslock_isr *lock)
```

Routine which initializes an mcslock_isr at runtime

---
```
void mcs_lock_isr_save(struct mcslock_isr *lock, struct mcsnode *node)
```

Disables interrupts and locks an mcslock, busy waits in the contended case

---
```
void mcs_unlock_isr_restore(struct mcslock_isr *lock, struct mcsnode *node)
```

Unlocks an mcslock, Reenables interrupts if it was enabled before the critical section started

---
```
bool mcs_trylock_isr_save(struct mcslock_isr *lock, struct mcsnode *node)
```

Routine which attempts to acquire an mcs lock_isr, returns true on acquisition otherwise false