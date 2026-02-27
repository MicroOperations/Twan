#ifndef _DELTA_CHAIN_H_
#define _DELTA_CHAIN_H_

#include <lib/dsa/dq.h>

struct delta_node
{
    u64 delta;
    struct list_double node;
};

typedef void (*delta_chain_dequeue_func_t)(struct delta_node *node);

struct delta_chain 
{
    struct dq dq;
};

#define INITIALIZE_DELTA_CHAIN()    \
{                                   \
    .dq.front = NULL,               \
    .dq.rear = NULL,                \
}

#define node_to_delta_node(ptr) container_of((ptr), struct delta_node, node)

inline void delta_chain_init(struct delta_chain *chain)
{
    chain->dq.front = NULL;
    chain->dq.rear = NULL;
}

inline void delta_chain_insert(struct delta_chain *chain, 
                              struct delta_node *node, u64 ticks)
{
    node->node.prev = NULL;
    node->node.next = NULL;

    if (dq_is_empty(&chain->dq)) {
        node->delta = ticks;
        dq_pushfront(&chain->dq, &node->node);
        return;
    }

    u64 remaining = ticks;

    struct list_double *cur = chain->dq.front;
    struct list_double *prev = NULL;

    while (cur) {

        struct delta_node *cur_node = node_to_delta_node(cur);
        if (remaining < cur_node->delta) {
            cur_node->delta -= remaining;
            break;
        }

        remaining -= cur_node->delta;
    
        prev = cur;
        cur = cur->next;
    }

    node->delta = remaining;
    dq_insert(&chain->dq, &node->node, prev, cur);
}

inline void delta_chain_advance(struct delta_chain *chain, 
                                delta_chain_dequeue_func_t func)
{
    struct list_double *cur = NULL;
    while ((cur = dq_peekfront(&chain->dq))) {

        struct delta_node *node = node_to_delta_node(cur);
        if (node->delta != 0)
            break;

        dq_popfront(&chain->dq);

        if (func)
            func(node);
    }
}

inline void delta_chain_tick(struct delta_chain *chain, 
                             delta_chain_dequeue_func_t func)
{
    if (dq_is_empty(&chain->dq))
        return;

    struct delta_node *node = node_to_delta_node(chain->dq.front);

    if (node->delta == 0 || --node->delta == 0)
        delta_chain_advance(chain, func);
}

inline void delta_chain_dequeue_no_callback(struct delta_chain *chain,
                                            struct delta_node *node)
{
    if (node->node.next) {
        struct delta_node *next_node = node_to_delta_node(node->node.next);
        next_node->delta += node->delta;
    }

    dq_dequeue(&chain->dq, &node->node);
}

inline bool delta_chain_is_queued(struct delta_chain *chain, 
                                  struct delta_node *node)
{
    return dq_is_queued(&chain->dq, &node->node);
}

inline bool delta_chain_is_empty(struct delta_chain *chain)
{
    return dq_is_empty(&chain->dq);
}

inline struct delta_node *delta_chain_peekfront(struct delta_chain *chain)
{
    struct list_double *node = dq_peekfront(&chain->dq);
    return node ? node_to_delta_node(node) : NULL;
}

inline bool delta_chain_is_front(struct delta_chain *chain, 
                                 struct delta_node *node)
{
    return delta_chain_peekfront(chain) == node;
}

inline struct delta_node *delta_chain_popfront_noupdate(
    struct delta_chain *chain)
{
    struct list_double *node = dq_popfront(&chain->dq);
    return node ? node_to_delta_node(node) : NULL;
}

#endif