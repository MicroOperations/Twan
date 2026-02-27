#ifndef _DQ_H_
#define _DQ_H_

#include <std.h>

#define PUSHFRONT true
#define PUSHBACK false

struct list_double
{
    struct list_double *next;
    struct list_double *prev;
};

struct dq 
{
    struct list_double *front;
    struct list_double *rear;
};

inline void dq_pushfront(struct dq *dq, struct list_double *node)
{
    node->prev = NULL;

    node->next = dq->front;
    dq->front = node;

    if (node->next)
        node->next->prev = node;
    else 
        dq->rear = node;
}

inline void dq_pushback(struct dq *dq, struct list_double *node)
{
    node->next = NULL;

    node->prev = dq->rear;
    dq->rear = node;

    if (node->prev)
        node->prev->next = node;
    else 
        dq->front = node;
}

inline void dq_push(struct dq *dq, struct list_double *node, int direction)
{
    if (direction == PUSHFRONT)
        dq_pushfront(dq, node);
    else 
        dq_pushback(dq, node);
}

inline void dq_dequeue(struct dq *dq, struct list_double *node)
{
    if (node->next) 
        node->next->prev = node->prev;
    else
        dq->rear = node->prev;

    if (node->prev)
        node->prev->next = node->next;
    else
        dq->front = node->next;

    node->next = NULL;
    node->prev = NULL;
}

inline struct list_double *dq_peekfront(struct dq *dq)
{
    return dq->front;
}

inline struct list_double *dq_popfront(struct dq *dq)
{
    struct list_double *node = dq->front;
    if (!node)
        return NULL;

    dq->front = node->next;
    if (dq->front)
        dq->front->prev = NULL;
    else 
        dq->rear = NULL;

    node->prev = NULL;
    node->next = NULL;
    return node;
}

inline void dq_insert(struct dq *dq, struct list_double *node, 
                      struct list_double *prev, struct list_double *next)
{
    node->prev = prev;
    node->next = next;

    if (next)
        next->prev = node;
    else 
        dq->rear = node;

    if (prev)
        prev->next = node;
    else
        dq->front = node;
}

inline bool dq_is_queued(struct dq *dq, struct list_double *node)
{
    return node->prev || node->next || dq->front == node;
}

inline bool dq_is_empty(struct dq *dq)
{
    return !dq->front;
}

inline void dq_clear(struct dq *dq)
{
    struct list_double *cur = dq->front;

    while (cur) {

        struct list_double *next = cur->next;

        cur->prev = NULL;
        cur->next = NULL;
        cur = next;
    }

    dq->front = NULL;
    dq->rear = NULL;
}

#endif