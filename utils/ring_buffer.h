#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "common_types.h"

#define RING_BUF_SIZE 256

typedef struct {
    u8  buffer[RING_BUF_SIZE];
    volatile u16 head;   /* ISR 写入（push），主循环只读 */
    volatile u16 tail;   /* 主循环写入（pop），ISR 只读 */
    volatile u16 count;
} RingBuffer;

void      ringbuf_init(RingBuffer* rb);
bool      ringbuf_is_empty(RingBuffer* rb);
bool      ringbuf_is_full(RingBuffer* rb);
RetStatus ringbuf_push(RingBuffer* rb, u8 data);
RetStatus ringbuf_pop(RingBuffer* rb, u8* data);
u16       ringbuf_available(RingBuffer* rb);
RetStatus ringbuf_write(RingBuffer* rb, const u8* data, u16 len);
u16       ringbuf_read(RingBuffer* rb, u8* data, u16 max_len);

#endif /* RING_BUFFER_H */
