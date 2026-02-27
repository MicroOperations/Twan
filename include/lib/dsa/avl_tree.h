#ifndef _AVL_TREE_H_
#define _AVL_TREE_H_

#include <lib/dsa/linked_tree.h>

typedef void (*avl_tree_remove_func_t)(struct tree_node *node, void *data);

struct avl_tree
{
    struct tree_node *root;
    u32 max_path;
};

#define INITIALIZE_AVL_TREE(_max_path)  \
{                                       \
    .root = NULL,                       \
    .max_path = (_max_path)             \
}

#define avl_tree_node_key(node) ((node)->data & 0xffffffff)
#define avl_tree_node_height(node) ((node)->data >> 32)

/* (1.5 * log2(num_nodes) + 2) > ((1.44 * log2(num_nodes) + 2) - 0.328) */
#define avl_tree_max_path(num_nodes) \
    ((num_nodes) ? ((log2_ceil32(num_nodes) * 3) / 2) + 2 : 0)

#define avl_height(node) \
    ((node) ? avl_tree_node_height(node) : 0)

inline void avl_tree_init(struct avl_tree *tree, u32 max_path)
{
    tree->root = NULL;
    tree->max_path = max_path;
}

inline int avl_balance_factor(struct tree_node *node)
{
    return node ? avl_height(node->left) - avl_height(node->right) : 0;
}

inline void avl_update_height(struct tree_node *node)
{
    node->data = 
        (max(avl_height((node)->left), avl_height((node)->right)) + 1) << 32;
}

inline struct tree_node *avl_rotate_right(struct tree_node *node)
{
    struct tree_node *left_child = node->left;
    struct tree_node *left_child_right = left_child->right;

    left_child->right = node;
    node->left = left_child_right;

    left_child->parent = node->parent;
    node->parent = left_child;

    if (left_child_right)
        left_child_right->parent = node;

    avl_update_height(node);
    avl_update_height(left_child);

    return left_child;
}

inline struct tree_node *avl_rotate_left(struct tree_node *node)
{
    struct tree_node *right_child = node->right;
    struct tree_node *right_child_left = right_child->left;

    right_child->left = node;
    node->right = right_child_left;

    right_child->parent = node->parent;
    node->parent = right_child;

    if (right_child_left)
        right_child_left->parent = node;

    avl_update_height(node);
    avl_update_height(right_child);

    return right_child;
}

inline struct tree_node *avl_find_highest(struct avl_tree *tree)
{
    struct tree_node *root = tree->root;

    if (!root)
        return NULL;

    struct tree_node *cur = root;
    while (cur->right != NULL) 
        cur = cur->right;

    return cur;
}

inline struct tree_node *avl_search(struct avl_tree *tree, u32 key)
{
    struct tree_node *cur = tree->root;

    while (cur != NULL) {

        u32 cur_key = avl_tree_node_key(cur);

        if (cur_key == key)
            return cur;
        else if (cur_key > key)
            cur = cur->left;
        else
            cur = cur->right;
    }   

    return NULL;
}

bool avl_insert(struct avl_tree *tree, struct tree_node *node, u32 key,
                struct tree_node **duplicate);

bool avl_remove(struct avl_tree *tree, struct tree_node *node);

inline void avl_replace(struct avl_tree *tree, struct tree_node *dest, 
                        struct tree_node *src)
{
    struct tree_node *parent = dest->parent;

    src->data = dest->data;
    src->left = dest->left;
    src->right = dest->right;
    src->parent = parent;

    if (src->left)
        src->left->parent = src;
    
    if (src->right)
        src->right->parent = src;

    if (parent) {

        if (parent->left == dest)
            parent->left = src;
        else
            parent->right = src;

    } else {
        tree->root = src;
    }

    dest->data = 0;
    dest->left = NULL;
    dest->right = NULL;
    dest->parent = NULL;
}

inline bool avl_is_queued(struct avl_tree *tree, struct tree_node *src)
{
    return src->parent || tree->root == src;
}

inline void avl_remove_all(struct avl_tree *tree, avl_tree_remove_func_t func, 
                           void *data)
{
    struct tree_node *stack[tree->max_path];
    u32 len = 0;

    struct tree_node *cur = tree->root;

    while (len > 0 || cur) {

        while (cur) {
            stack[len++] = cur;
            cur = cur->right;
        }

        cur = stack[--len];

        struct tree_node *left = cur->left;

        cur->parent = NULL;
        cur->left = NULL;
        cur->right = NULL;
        cur->data = 0;

        func(cur, data);

        cur = left;
    }

    tree->root = NULL;
}

#endif