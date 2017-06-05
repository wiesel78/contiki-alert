#ifndef QUEUE_H_
#define QUEUE_H_

#include "contiki.h"
#include "lib/memb.h"

typedef struct data_queue_item {
    void *data_item;
    struct data_queue_item *next;
} data_queue_item_t;

typedef struct data_queue {
    data_queue_item_t *first;
    data_queue_item_t *last;
    uint8_t count;
    uint8_t max;
    struct memb *queue_memb;
    struct memb *data_memb;
    void *buffer;
} data_queue_t;


extern void
data_queue_init(data_queue_t *queue, struct memb *queue_memb, struct memb *data_memb);

extern void*
data_queue_enqueue(data_queue_t *queue, void *data);

extern void*
data_queue_peek(data_queue_t *queue);

extern void*
data_queue_dequeue(data_queue_t *queue);

#endif /* QUEUE_H_ */
