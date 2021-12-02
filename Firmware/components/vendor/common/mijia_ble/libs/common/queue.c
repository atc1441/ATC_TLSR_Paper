#include <string.h>
#include "mible_type.h"
#include "queue.h"

mible_status_t queue_init(queue_t *q, void *buf, char queue_size, char elem_size)
{
    if (buf == NULL || q == NULL)
        return MI_ERR_INVALID_PARAM;

    q->buf = buf;
    q->size = queue_size;
    q->offset = elem_size;
    q->rd_ptr = 0;
    q->wr_ptr = 0;
    q->used = 0;

    return MI_SUCCESS;
}

mible_status_t enqueue(queue_t *q, void *in)
{
    if (q->used == q->size) {
        return MI_ERR_NO_MEM;
    }
    
    /* q->buf[q->wr_ptr++] = in; */
    memcpy((char*)q->buf + q->wr_ptr * q->offset, in, q->offset);
    q->wr_ptr = (q->wr_ptr + 1) % q->size;
    q->used++;
    
    return MI_SUCCESS;
}

mible_status_t dequeue(queue_t *q, void *out)
{
    if (q->used > 0) {
        /* *out = q->buf[q->rd_ptr++]; */
        memcpy(out, (char*)q->buf + q->rd_ptr * q->offset, q->offset);
        q->rd_ptr = (q->rd_ptr + 1) % q->size;
        q->used--;
        return MI_SUCCESS;
    } else
        return MI_ERR_NOT_FOUND;
}

void queue_flush(queue_t *q)
{
    q->rd_ptr = 0;
    q->wr_ptr = 0;
    q->used = 0;
}
