#ifndef _LISTQ_H_
#define _LISTQ_H_

#include <std.h>

struct listq
{
    struct listq *prev;
    struct listq *next;
    struct listq *rear;
    struct listq *front;
};

typedef void (*listq_disconnect_func_t)(struct listq *node, void *data);

inline void listq_pushback(struct listq *front, struct listq *node)
{
    node->prev = NULL;
    node->next = NULL;
    node->rear = NULL;
    node->front = NULL;

    if (front->rear) {
        front->rear->front = NULL;
        front->rear->next = node;
        node->prev = front->rear;
    } else {
        front->next = node;
        node->prev = front;
    }

    node->front = front;
    front->rear = node;
}

inline struct listq *listq_disconnect(struct listq *node)
{
    struct listq *prev = node->prev;
    struct listq *next = node->next;
    struct listq *rear = node->rear;
    struct listq *front = node->front;

    if (front) {

        if (node->prev != front) {
            node->prev->front = front;
            front->rear = node->prev;
        } else {
            front->rear = NULL;
        }

        node->prev->next = NULL;

    } else if (rear) {

        if (node->next != rear) {
            node->next->rear = rear;
            rear->front = node->next;
        } else {
            rear->front = NULL;
        }

        node->next->prev = NULL;

    } else {

        if (next) {

            if (prev) {

                next->prev = prev;
                prev->next = next;

            } else {
                next->prev = NULL;
            }

        } else if (prev) {
            prev->next = NULL;
        }
    }

    node->prev = NULL;
    node->next = NULL;
    node->rear = NULL;
    node->front = NULL;

    return next;
}

inline void listq_disconnect_all(struct listq *front, 
                                listq_disconnect_func_t func, void *data)
{
    while (front) {

        struct listq *next = front->next;

        front->prev = NULL;
        front->next = NULL;
        front->rear = NULL;
        front->front = NULL;

        func(front, data);

        front = next;
    }
}

#endif