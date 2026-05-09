#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include "common_types.h"
#include "ring_buffer.h"

#define UART_BAUDRATE 115200

void uart_module_init(void);
void uart_send_byte(u8 data);
void uart_send_string(const char* str);
void uart_send_buffer(const u8* buf, u16 len);
RingBuffer* uart_get_rx_buffer(void);

#endif /* UART_DRIVER_H */
