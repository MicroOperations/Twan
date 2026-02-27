#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <subsys/sync/semaphore.h>

struct mutex_ipcp
{
    u8 priority_ceiling;
    u8 criticality_ceiling;

    u8 last_priority;
    u8 last_criticality;

    atomic_ptr_t holder;
    u64 count;
    struct waitq waitq;
};

#define INITIALIZE_MUTEX_IPCP(_priority_ceiling, _criticality_ceiling)      \
{                                                                           \
    .priority_ceiling = (_priority_ceiling),                                \
    .criticality_ceiling = (_criticality_ceiling),                          \
    .last_priority = 0,                                                     \
    .last_criticality = 0,                                                  \
    .holder = INITIALIZE_ATOMIC_PTR(NULL),                                  \
    .count = 0,                                                             \
    .waitq = INITIALIZE_WAITQ()                                             \
};                                                                          \
STATIC_ASSERT((_criticality_ceiling) < SCHED_NUM_CRITICALITIES)

int mutex_ipcp_init(struct mutex_ipcp *mutex_ipcp, u8 priority_ceiling, 
                    u8 criticality_ceiling);

void mutex_ipcp_lock(struct mutex_ipcp *mutex_ipcp);
bool mutex_ipcp_lock_timeout(struct mutex_ipcp *mutex_ipcp, u32 ticks);
void mutex_ipcp_unlock(struct mutex_ipcp *mutex_ipcp);
bool mutex_ipcp_trylock(struct mutex_ipcp *mutex_ipcp);

#endif