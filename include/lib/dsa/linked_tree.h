#ifndef _LINKED_TREE_H_
#define _LINKED_TREE_H_

#include <std.h>

struct tree_node
{
    u64 data;
    struct tree_node *left;
    struct tree_node *right;
    struct tree_node *parent;
};

#endif