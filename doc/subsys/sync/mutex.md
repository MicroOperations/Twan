# Mutex

Mutexes can be leveraged to synchronise threads in a manner where waiters can block, and the owner can be preempted. Currently, mutexes use the ipcp (immediate priority ceiling protocol) to prevent inversion, are reentrant and unlocks will atomically handoff ownership to the task with the highest real priority. During unlock operations, the tasks priority is set to its priority prior to acquiring the ipcp mutex, meaning that tasks should ensure they release mutexes in reverse order to which they were acquired to prevent inversion. If multiple tasks are at the highest priority, ownership is handed to whoever has been in the waitq associated with the task the longest.

## API's

```<subsys/sync/mutex.h>```

---
```
INITIALIZE_MUTEX_IPCP(_priority_ceiling, _criticality_ceiling)
```

Macro used to initialize ipcp mutexes, returns 0 on success, otherwise an
errcode

---
```
int mutex_ipcp_init(struct mutex_ipcp *mutex_ipcp, u8 priority_ceiling, 
                    u32 criticality_ceiling)
```

Routine used to initialize ipcp mutexes at runtime

---
```
void mutex_ipcp_lock(struct mutex_ipcp *mutex_ipcp)
```

Routine to lock an ipcp mutex, lock acquisitions are reentrant, and will increment a counter internally

---
```
bool mutex_ipcp_lock_timeout(struct mutex_ipcp *mutex_ipcp, u32 ticks)
```

Routine to lock an ipcp mutex with a timeout, returns true on successful acquisition, otherwise it returns false

---
```
void mutex_ipcp_unlock(struct mutex_ipcp *mutex_ipcp)
```

Routine which decrements the counter associated with the ipcp mutex, once it hits 0, it'll be unlocked, in the case any tasks are waiting for the lock, the mutex will be atomically handed off to the highest real priority waiting task that has been in the acquisition queue the longest

---
```
bool mutex_ipcp_trylock(struct mutex_ipcp *mutex_ipcp)
```

Routine which attempts to acquire an ipcp mutex, returns true on successful acquisition, otherwise it returns false