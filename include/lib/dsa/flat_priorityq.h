#ifndef _FLAT_PRIORITYQ_H_
#define _FLAT_PRIORITYQ_H_

#include <lib/dsa/listq.h>
#include <lib/dsa/avl_tree.h>

struct flat_priorityq_node
{
    struct tree_node tree_node;
    struct listq listq;
};

typedef void (*flat_priorityq_dequeue_func_t)(struct flat_priorityq_node *node);

struct flat_priorityq
{
    /* using an avl tree so that the wcet is bounded by that of 256 priority
       levels even if the number of nodes surpasses that, ensuring better 
       scalability than min-heaps */

    struct avl_tree tree;
};

#define INITIALIZE_FLAT_PRIORITYQ()                         \
{                                                           \
    .tree = INITIALIZE_AVL_TREE(avl_tree_max_path(256))     \
}

#define tree_node_to_flat_priorityq_node(ptr) \
    container_of((ptr), struct flat_priorityq_node, tree_node)

#define listq_to_flat_priorityq_node(ptr) \
    container_of((ptr), struct flat_priorityq_node, listq)

inline void flat_priorityq_init(struct flat_priorityq *flat_priorityq)
{
    avl_tree_init(&flat_priorityq->tree, avl_tree_max_path(256));
}

inline void flat_priorityq_insert(struct flat_priorityq *flat_priorityq, 
                                  struct flat_priorityq_node *node, u8 priority)
{
    struct avl_tree *tree = &flat_priorityq->tree;

    struct tree_node *duplicate;
    if (!avl_insert(tree, &node->tree_node, priority, &duplicate)) {

        struct flat_priorityq_node *flat_duplicate = 
            tree_node_to_flat_priorityq_node(duplicate);

        listq_pushback(&flat_duplicate->listq, &node->listq);
    }
}

inline void flat_priorityq_dequeue(struct flat_priorityq *flat_priorityq, 
                                   struct flat_priorityq_node *node)
{
    struct avl_tree *tree = &flat_priorityq->tree;
    struct tree_node *tree_node = &node->tree_node;

    struct listq *listq_next = listq_disconnect(&node->listq);

    if (avl_is_queued(&flat_priorityq->tree, &node->tree_node)) {

        if (!listq_next) {
            avl_remove(tree, tree_node);
            return;
        }

        struct flat_priorityq_node *next_node = 
            listq_to_flat_priorityq_node(listq_next);

        avl_replace(tree, tree_node, &next_node->tree_node);
    }
}

inline struct flat_priorityq_node *flat_priorityq_popfront(
    struct flat_priorityq *flat_priorityq)
{
    struct avl_tree *tree = &flat_priorityq->tree;

    struct tree_node *tree_node = avl_find_highest(tree);
    if (!tree_node)
        return NULL;

    struct flat_priorityq_node *node = 
        tree_node_to_flat_priorityq_node(tree_node);

    flat_priorityq_dequeue(flat_priorityq, node);
    return node;
}

inline bool flat_priorityq_is_queued(
    struct flat_priorityq *flat_priorityq, 
    struct flat_priorityq_node *node)
{
    return avl_is_queued(&flat_priorityq->tree, &node->tree_node) || 
            node->listq.prev;
}

inline void flat_priorityq_listq_remove_func(struct listq *node, void *data)
{
    flat_priorityq_dequeue_func_t func = data;

    struct flat_priorityq_node *flat_priorityq_node = 
        listq_to_flat_priorityq_node(node);

    func(flat_priorityq_node);
}

inline void flat_priorityq_avl_remove_func(struct tree_node *node, void *data)
{
    flat_priorityq_dequeue_func_t func = data;

    struct flat_priorityq_node *flat_priorityq_node = 
        tree_node_to_flat_priorityq_node(node);

    listq_disconnect_all(&flat_priorityq_node->listq, 
                         flat_priorityq_listq_remove_func, func);
}

inline void flat_priorityq_dequeue_all(struct flat_priorityq *flat_priorityq, 
                                       flat_priorityq_dequeue_func_t func)
{
    avl_remove_all(&flat_priorityq->tree, flat_priorityq_avl_remove_func, func);
}

#endif