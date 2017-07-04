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

/* Initialize the Queue
 * queue is seperate in queue items and data items
 * data items are the data of interest for you
 * queue items wrap the data items and administrated by queue
 * @example
 *      #include "lib/memb.h"
 *      MEMB(publish_queue_items, data_queue_item_t, 16);
 *      MEMB(publish_data_items, publish_item_t, 17)
 * @param queue : Pointer of queue to initialize
 * @param queue_memb : memb pointer for space of queue wrapper items
 * @param data_memb : memb pointer for space of queue data items */
extern void
data_queue_init(data_queue_t *queue, struct memb *queue_memb, struct memb *data_memb);

/* add a queue item to the queue
 * @param queue : Pointer to queue
 * @param data : Pointer to data to add to queue
 * @return Pointer of last item of this queue */
extern void*
data_queue_enqueue(data_queue_t *queue, void *data);

/* get the first item without delete it
 * @param queue : Pointer of queue
 * @return Pointer to queue-buffer with data of first item*/
extern void*
data_queue_peek(data_queue_t *queue);

/* get the first item and delete it
 * @param queue : Pointer of queue
 * @return Pointer to queue-buffer with data of first item*/
extern void*
data_queue_dequeue(data_queue_t *queue);

#endif /* QUEUE_H_ */
