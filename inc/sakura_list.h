/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_list.h
 * @brief header of list header structure
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */

#ifndef SAKURA_LIST_H
#define SAKURA_LIST_H

#include "sakura_types.h"

#if defined(_WIN32) && !defined(__cplusplus)
#define inline __inline
#endif

/*list_head*/
struct list_head {
    struct list_head *next, *prev;
};

typedef struct{
    sakura_uint8_t* data;
    sakura_uint32_t len;
} queue_string_data_t;

typedef struct list_head list_head_t;

/* queue */
typedef struct queue_node_t {
    queue_string_data_t data;
    struct list_head list;
} queue_node_t;

typedef struct queue_t {
    struct list_head head;
} queue_t;


/**
 * list_for_each    -   iterate over a list
 * @pos:    the &struct list_head to use as a loop counter.
 * @head:   the head for your list.
 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * @brief init list
 */
#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (SAKURA_FALSE)

/**
 * @brief get a list node
 */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * @brief ergodic list
 */
#define list_for_each_safe(pos, n, head) \
    for ((pos) = (head)->next, (n) = (pos)->next; (pos) != (head); \
        (pos) = (n) , (n)  = (pos)->next)

/**
 * @brief check list is empty
 * 
 */
#define list_empty(head) \
    ((head) == (head)->prev && (head) == (head)->next)

/**
 * @brief insert item between prev and next of list
 *
 * @param[in] newitem new item to insert
 * @param[in] prev prev item
 * @param[in] next next item
 * @return sakura_void_t
 */
static inline sakura_void_t list_add_inner(struct list_head * newitem,
    struct list_head * prev,
    struct list_head * next)
{
    next->prev = newitem;
    newitem->next = next;
    newitem->prev = prev;
    prev->next = newitem;
}

/**
 * @brief add item to list tail
 *
 * @param[in] newitem new item to insert
 * @param[in] head list head
 * @return sakura_void_t
 */
static inline sakura_void_t list_add_tail(struct list_head *newitem, struct list_head *head)
{
    list_add_inner(newitem, head->prev, head);
}

/**
 * @brief delete inner item in list
 *
 * @param[in] prev prev item
 * @param[in] next next item
 * @return sakura_void_t
 */
static inline sakura_void_t list_del_inner(struct list_head * prev,
                  struct list_head * next)
{
    /* change the prev and next pointer */
    next->prev = prev;
    prev->next = next;
}

/**
 * @brief delete item in list
 *
 * @param[in] entry item to delete
 * @return sakura_void_t
 */
static inline sakura_void_t list_del(struct list_head *entry)
{
    /* call list_del_inner */
    list_del_inner(entry->prev, entry->next);
}


static inline sakura_void_t init_queue(queue_t *queue) {
    INIT_LIST_HEAD(&queue->head);
}

/**
 * @brief queue is empty
 * 
 * @param queue queue
 * @return int 
 */
static inline sakura_int32_t queue_is_empty(queue_t *queue) {
    return list_empty(&queue->head);
}

/**
 * @brief enqueue
 * 
 * @param queue queue
 * @param node new node
 */
static inline sakura_void_t enqueue(queue_t *queue, queue_node_t *node) {
    list_add_tail(&node->list, &queue->head);
}

/**
 * @brief dequeue
 * 
 * @param queue queue
 * @return queue_node_t* 
 */
static inline queue_node_t* dequeue(queue_t *queue) {
    queue_node_t *node = NULL;
    if (!queue_is_empty(queue)) {
        node = list_entry(queue->head.next, queue_node_t, list);
        list_del(&node->list);
    }

    return node;
}

#endif

