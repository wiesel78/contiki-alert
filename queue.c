#include "contiki.h"
#include "lib/memb.h"
#include "./queue.h"


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
void
data_queue_init(data_queue_t *queue,
                struct memb *queue_memb,
                struct memb *data_memb){
    queue->first = NULL;
    queue->last = NULL;
    queue->queue_memb = queue_memb;
    queue->data_memb = data_memb;
    queue->buffer = memb_alloc(data_memb);
}

/* add a queue item to the queue
 * @param queue : Pointer to queue
 * @param data : Pointer to data to add to queue
 * @return Pointer of last item of this queue */
void*
data_queue_enqueue(data_queue_t *queue, void *data){
    data_queue_item_t *item;

    // stop if no free space for new item
    item = memb_alloc(queue->queue_memb);
    if(item == NULL){
        return NULL;
    }

    // fill data into item
    item->next = NULL;
    item->data_item = memb_alloc(queue->data_memb);

    if(item->data_item == NULL){
        memb_free(queue->queue_memb, item);
        return NULL;
    }

    // copy data in its own space
    memcpy(item->data_item, data, queue->data_memb->size);

    if(queue->first == NULL){
        // if first item, set first and last to same
        queue->first = item;
        queue->last = item;
    }else{
        // if not the first, set the last pointer to new item
        queue->last->next = item;
        queue->last = item;
    }

    return queue->last->data_item;
}

/* get the first item without delete it
 * @param queue : Pointer of queue
 * @return Pointer to queue-buffer with data of first item*/
void*
data_queue_peek(data_queue_t *queue){
    return queue->first;
}

/* get the first item and delete it
 * @param queue : Pointer of queue
 * @return Pointer to queue-buffer with data of first item*/
void*
data_queue_dequeue(data_queue_t *queue){
    data_queue_item_t *item;

    // save address of first item
    item = queue->first;

    // catch error
    if(item == NULL){
        return NULL;
    }

    // if next of this item NULL, first == last item
    if(item->next == NULL){
        queue->last = NULL;
    }

    // first is deleting and the next item is the new first item
    queue->first = item->next;

    // copy data of old first item in the buffer
    memcpy(queue->buffer, item->data_item, queue->data_memb->size);

    // deallocate space
    memb_free(queue->data_memb, item->data_item);
    memb_free(queue->queue_memb, item);

    // get the address of buffer with data of old first item
    return queue->buffer;
}
