#ifndef _PRIORITYQ_H_
#define _PRIORITYQ_H_

#include <lib/dsa/dq.h>
#include <lib/dsa/bmp256.h>

#define MIN_PRIORITY 0
#define MAX_PRIORITY 255

#define LOWEST_PRIORITY MIN_PRIORITY
#define HIGHEST_PRIORITY MAX_PRIORITY

struct priorityq
{
    struct dq priority_bins[MAX_PRIORITY + 1];
    struct bmp256 priority_bitmap;
};

inline void priorityq_set_priority(struct priorityq *priorityq, u8 priority)
{
    bmp256_set(&priorityq->priority_bitmap, priority);   
}

inline void priorityq_unset_priority(struct priorityq *priorityq, u8 priority)
{
    bmp256_unset(&priorityq->priority_bitmap, priority);
}

inline int priorityq_awaiting_priority(struct priorityq *priorityq)
{
    return bmp256_fls(&priorityq->priority_bitmap);
}

inline void priorityq_pushfront(struct priorityq *priorityq,
                                struct list_double *node, u8 priority)
{
    dq_pushfront(&priorityq->priority_bins[priority], node);
    priorityq_set_priority(priorityq, priority);
}

inline void priorityq_pushback(struct priorityq *priorityq, 
                              struct list_double *node, u8 priority)
{
    dq_pushback(&priorityq->priority_bins[priority], node);
    priorityq_set_priority(priorityq, priority);
}

inline void priorityq_push(struct priorityq *priorityq, 
                           struct list_double *node, u8 priority, 
                           bool direction)
{
    dq_push(&priorityq->priority_bins[priority], node, direction);
    priorityq_set_priority(priorityq, priority);
}

inline void priorityq_dequeue(struct priorityq *priorityq, 
                              struct list_double *node, u8 priority)
{
    struct dq *dq = &priorityq->priority_bins[priority];

    dq_dequeue(dq, node);

    if (dq_is_empty(dq))
        priorityq_unset_priority(priorityq, priority);
}

inline struct list_double *priorityq_peekfront(struct priorityq *priorityq)
{
    int priority = priorityq_awaiting_priority(priorityq);      
    if (priority == -1)
        return NULL;

    return dq_peekfront(&priorityq->priority_bins[priority]);
}

inline struct list_double *priorityq_popfront(struct priorityq *priorityq)
{
    int priority = priorityq_awaiting_priority(priorityq);      
    if (priority == -1)
        return NULL;

    struct dq *dq = &priorityq->priority_bins[priority];
    struct list_double *node = dq_popfront(dq);

    if (dq_is_empty(dq))
        priorityq_unset_priority(priorityq, priority);

    return node;
}

inline bool priorityq_is_queued(struct priorityq *priorityq, 
                                struct list_double *node, u8 priority)
{
    return dq_is_queued(&priorityq->priority_bins[priority], node);
}

inline void priorityq_requeue(struct priorityq *priorityq, 
                              struct list_double *node, u8 new_priority, 
                              u8 old_priority, bool direction)
{
    priorityq_dequeue(priorityq, node, old_priority);
    priorityq_push(priorityq, node, new_priority, direction);
}

#endif