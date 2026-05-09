#include "ring_buffer.h"

void ringbuf_init(RingBuffer* rb)
{
    if (rb == NULL) {
        return;
    }
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool ringbuf_is_empty(RingBuffer* rb)
{
    return (rb != NULL) ? (rb->count == 0) : true;
}

bool ringbuf_is_full(RingBuffer* rb)
{
    return (rb != NULL) ? (rb->count == RING_BUF_SIZE) : false;
}

RetStatus ringbuf_push(RingBuffer* rb, u8 data)
{
    if (rb == NULL) {
        return RET_INVALID_PARAM;
    }
    
    if (ringbuf_is_full(rb)) {
        return RET_NO_MEMORY;
    }
    
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % RING_BUF_SIZE;
    rb->count++;
    return RET_OK;
}

RetStatus ringbuf_pop(RingBuffer* rb, u8* data)
{
    if (rb == NULL || data == NULL) {
        return RET_INVALID_PARAM;
    }
    
    if (ringbuf_is_empty(rb)) {
        return RET_ERR;
    }
    
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUF_SIZE;
    rb->count--;
    return RET_OK;
}

u16 ringbuf_available(RingBuffer* rb)
{
    return (rb != NULL) ? rb->count : 0;
}

RetStatus ringbuf_write(RingBuffer* rb, const u8* data, u16 len)
{
    if (rb == NULL || data == NULL) {
        return RET_INVALID_PARAM;
    }
    
    for (u16 i = 0; i < len; i++) {
        if (ringbuf_push(rb, data[i]) != RET_OK) {
            return RET_ERR;
        }
    }
    return RET_OK;
}

u16 ringbuf_read(RingBuffer* rb, u8* data, u16 max_len)
{
    u16 count = 0;
    if (rb == NULL || data == NULL) {
        return 0;
    }
    
    while (count < max_len && !ringbuf_is_empty(rb)) {
        ringbuf_pop(rb, &data[count]);
        count++;
    }
    return count;
}
