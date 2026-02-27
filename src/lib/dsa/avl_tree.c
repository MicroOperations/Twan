#include <lib/dsa/avl_tree.h>

bool avl_insert(struct avl_tree *tree, struct tree_node *node, u32 key,
                struct tree_node **duplicate)
{
    *duplicate = NULL;

    node->data = (1ULL << 32) | key;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    if (!tree->root) {
        tree->root = node;
        return true;
    }

    long max_path = tree->max_path;

    struct tree_node *path[max_path];
    long path_len = 0;

    struct tree_node *cur = tree->root;
    struct tree_node *parent = NULL;

    while (cur != NULL) {

        if (path_len == max_path)
            return false;

        path[path_len++] = cur;
        parent = cur;

        u32 cur_key = avl_tree_node_key(cur);
        if (cur_key > key) {

            cur = cur->left;

        } else if (cur_key < key) {

            cur = cur->right;

        } else {
            *duplicate = cur;
            return false;
        }
    }

    if (key < avl_tree_node_key(parent))
        parent->left = node;
    else 
        parent->right = node;

    node->parent = parent;

    for (long i = path_len - 1; i >= 0; i--) {

        struct tree_node *path_node = path[i];
        struct tree_node *path_node_parent = i > 0 ? path[i - 1] : NULL;

        avl_update_height(path_node);

        int balance_factor = avl_balance_factor(path_node);

        struct tree_node *new_subtree = NULL;

        if (balance_factor > 1) {

            if (key > avl_tree_node_key(path_node->left)) {

                path_node->left = avl_rotate_left(path_node->left);
                new_subtree = avl_rotate_right(path_node);

            } else {
                new_subtree = avl_rotate_right(path_node);
            }

        } else if (balance_factor < -1) {

             if (key < avl_tree_node_key(path_node->right)) {

                path_node->right = avl_rotate_right(path_node->right);
                new_subtree = avl_rotate_left(path_node);

             } else {
                new_subtree = avl_rotate_left(path_node);
            }
        }

        if (new_subtree) {

            if (path_node_parent) {

                if (path_node_parent->left == path_node)
                    path_node_parent->left = new_subtree;
                else
                    path_node_parent->right = new_subtree;

                new_subtree->parent = path_node_parent;

            } else {
                tree->root = new_subtree;
                new_subtree->parent = NULL;
            }

        }
    } 

    return true;
}

bool avl_remove(struct avl_tree *tree, struct tree_node *node)
{
    if (!tree->root)
        return false;

    u32 key = avl_tree_node_key(node);

    struct tree_node *path[tree->max_path];
    long path_len = 0;

    struct tree_node *cur = tree->root;

    while (cur != NULL) {

        path[path_len++] = cur;

        u32 cur_key = avl_tree_node_key(cur);
        if (cur_key == key)
            break;

        if (cur_key > key)
            cur = cur->left;
        else 
            cur = cur->right;
    }

    if (!cur)
        return false;

    struct tree_node *parent = node->parent;

    if (!node->left || !node->right) {

        struct tree_node *child = node->left ? node->left : node->right;

        if (!parent)
            tree->root = child;
        else if (parent->left == node)
            parent->left = child;
        else
            parent->right = child;

        if (child)
            child->parent = parent;

        path_len--;

    } else {

        struct tree_node *succ_parent = node;
        struct tree_node *succ = node->right;

        u32 succ_path_start = path_len;
        while (succ->left != NULL) {
            path[path_len++] = succ;
            succ_parent = succ;
            succ = succ->left;
        }

        succ->left = node->left;
        if (node->left)
            node->left->parent = succ;

        if (succ_parent != node) {

            succ_parent->left = succ->right;
            if (succ->right)
                succ->right->parent = succ_parent;

            succ->right = node->right;
            if (node->right)
                node->right->parent = succ;
        }

        if (!parent)
            tree->root = succ;
        else if (parent->left == node)
            parent->left = succ;
        else
            parent->right = succ;

        succ->parent = parent;
        path[succ_path_start - 1] = succ;

        if (succ_path_start < path_len)
            path_len--;
    }

    node->data = 0;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    for (long i = path_len - 1; i >= 0; i--) {

        struct tree_node *path_node = path[i];
        struct tree_node *path_node_parent = i > 0 ? path[i - 1] : NULL;

        avl_update_height(path_node);

        int balance_factor = avl_balance_factor(path_node);

        struct tree_node *new_subtree = NULL;

        if (balance_factor > 1) {

            int left_balance = avl_balance_factor(path_node->left);

            if (left_balance < 0) {
            
                path_node->left = avl_rotate_left(path_node->left);
                new_subtree = avl_rotate_right(path_node);

            } else {
                new_subtree = avl_rotate_right(path_node);
            }

        } else if (balance_factor < -1) {

            int right_balance = avl_balance_factor(path_node->right);

            if (right_balance > 0) {

                path_node->right = avl_rotate_right(path_node->right);
                new_subtree = avl_rotate_left(path_node);

            } else {
                new_subtree = avl_rotate_left(path_node);
            }
        }

        if (new_subtree) {

            if (path_node_parent) {

                if (path_node_parent->left == path_node)
                    path_node_parent->left = new_subtree;
                else
                    path_node_parent->right = new_subtree;

                new_subtree->parent = path_node_parent;

            } else {

                tree->root = new_subtree;
                new_subtree->parent = NULL;
            }
        }
    }

    return true;
}