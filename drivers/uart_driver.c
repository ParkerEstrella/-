#include "uart_driver.h"
#include "gd32f4xx.h"
#include <stdio.h>

/* 串口配置 */
#define UART_PERIPH USART0
#define UART_CLK RCU_USART0
#define UART_TX_GPIO_PORT GPIOA
#define UART_TX_GPIO_PIN GPIO_PIN_9
#define UART_RX_GPIO_PORT GPIOA
#define UART_RX_GPIO_PIN GPIO_PIN_10
#define UART_GPIO_CLK RCU_GPIOA

/* 接收缓冲区 */
static RingBuffer rx_buffer;

/* 重定向 printf */
int fputc(int ch, FILE* f)
{
    uart_send_byte((u8)ch);
    return ch;
}

void uart_module_init(void)
{
    ringbuf_init(&rx_buffer);
    
    rcu_periph_clock_enable(UART_GPIO_CLK);
    rcu_periph_clock_enable(UART_CLK);
    
    /* 配置 TX */
    gpio_af_set(UART_TX_GPIO_PORT, GPIO_AF_7, UART_TX_GPIO_PIN);
    gpio_mode_set(UART_TX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART_TX_GPIO_PIN);
    gpio_output_options_set(UART_TX_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART_TX_GPIO_PIN);
    
    /* 配置 RX */
    gpio_af_set(UART_RX_GPIO_PORT, GPIO_AF_7, UART_RX_GPIO_PIN);
    gpio_mode_set(UART_RX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART_RX_GPIO_PIN);
    gpio_output_options_set(UART_RX_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART_RX_GPIO_PIN);
    
    /* 配置 USART */
    usart_deinit(UART_PERIPH);
    usart_baudrate_set(UART_PERIPH, UART_BAUDRATE);
    usart_word_length_set(UART_PERIPH, USART_WL_8BIT);
    usart_stop_bit_set(UART_PERIPH, USART_STB_1BIT);
    usart_parity_config(UART_PERIPH, USART_PM_NONE);
    usart_hardware_flow_rts_config(UART_PERIPH, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(UART_PERIPH, USART_CTS_DISABLE);
    usart_receive_config(UART_PERIPH, USART_RECEIVE_ENABLE);
    usart_transmit_config(UART_PERIPH, USART_TRANSMIT_ENABLE);
    
    /* 使能接收中断 */
    nvic_irq_enable(USART0_IRQn, 2, 0);
    usart_interrupt_enable(UART_PERIPH, USART_INT_RBNE);
    
    usart_enable(UART_PERIPH);
}

void uart_send_byte(u8 data)
{
    u32 timeout = 100000;
    while (usart_flag_get(UART_PERIPH, USART_FLAG_TBE) == RESET) {
        if (--timeout == 0) break;
    }
    usart_data_transmit(UART_PERIPH, data);
}

void uart_send_string(const char* str)
{
    if (str == NULL) return;
    while (*str) {
        uart_send_byte((u8)*str++);
    }
}

void uart_send_buffer(const u8* buf, u16 len)
{
    for (u16 i = 0; i < len; i++) {
        uart_send_byte(buf[i]);
    }
}

RingBuffer* uart_get_rx_buffer(void)
{
    return &rx_buffer;
}

/* 串口中断处理 */
void USART0_IRQHandler(void)
{
    if (usart_interrupt_flag_get(UART_PERIPH, USART_INT_FLAG_RBNE) != RESET) {
        u8 ch = usart_data_receive(UART_PERIPH);
        ringbuf_push(&rx_buffer, ch);
    }
}
