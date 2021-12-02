#ifndef __QUEUE_H_
#define __QUEUE_H_

#include <stdint.h>
#include "mible_type.h"

typedef struct {
    void * buf;
    uint8_t size;
    uint8_t offset;
    uint8_t rd_ptr;
    uint8_t wr_ptr;
    uint8_t used;
} queue_t;

mible_status_t queue_init(queue_t *q, void *buf, char size, char elem_size);
mible_status_t enqueue(queue_t *q, void *in);
mible_status_t dequeue(queue_t *q, void *out);
void queue_flush(queue_t *q);

#endif //__QUEUE_H_
