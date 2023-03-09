#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdbool.h>

/* Structs */

/* Simple queue struct. */
struct queue_t {
    void **items;
    int start;
    int end;
    int size; 
    int max;
};

/* Methods */

/* Method to allocate a new queue. */
struct queue_t *malloc_queue(int size);

/* Method to free queue. */
void free_queue(struct queue_t *queue);

/* Method to check if queue is empty */
bool queue_empty(struct queue_t *queue);

/* Method to check if queue is full. */
bool queue_full(struct queue_t *queue);

/* Method to add an item to the queue. */
bool queue_push(struct queue_t *queue, void *item);

/* Method to pop first item from queue. */
void *queue_pop(struct queue_t *queue);

#endif