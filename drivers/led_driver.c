#include "led_driver.h"
#include "gd32f4xx.h"

/* LED 引脚定义 */
#define LED1_GPIO_PORT GPIOB
#define LED1_GPIO_PIN GPIO_PIN_3
#define LED1_GPIO_CLK RCU_GPIOB

#define LED2_GPIO_PORT GPIOB
#define LED2_GPIO_PIN GPIO_PIN_4
#define LED2_GPIO_CLK RCU_GPIOB

#define LED3_GPIO_PORT GPIOB
#define LED3_GPIO_PIN GPIO_PIN_5
#define LED3_GPIO_CLK RCU_GPIOB

#define LED4_GPIO_PORT GPIOB
#define LED4_GPIO_PIN GPIO_PIN_6
#define LED4_GPIO_CLK RCU_GPIOB

/* 引脚表 */
static const struct {
    uint32_t port;
    uint32_t pin;
    rcu_periph_enum clk;
} led_info[LED_ID_COUNT] = {
    {LED1_GPIO_PORT, LED1_GPIO_PIN, LED1_GPIO_CLK},
    {LED2_GPIO_PORT, LED2_GPIO_PIN, LED2_GPIO_CLK},
    {LED3_GPIO_PORT, LED3_GPIO_PIN, LED3_GPIO_CLK},
    {LED4_GPIO_PORT, LED4_GPIO_PIN, LED4_GPIO_CLK},
};

void led_initialize_all(void)
{
    for (u8 i = 0; i < LED_ID_COUNT; i++) {
        rcu_periph_clock_enable(led_info[i].clk);
        gpio_mode_set(led_info[i].port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, led_info[i].pin);
        gpio_output_options_set(led_info[i].port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, led_info[i].pin);
        gpio_bit_reset(led_info[i].port, led_info[i].pin);
    }
}

void led_set_state(LedId id, LedState state)
{
    if (id >= LED_ID_COUNT) {
        return;
    }
    
    switch (state) {
        case LED_STATE_ON:
            gpio_bit_set(led_info[id].port, led_info[id].pin);
            break;
        case LED_STATE_OFF:
            gpio_bit_reset(led_info[id].port, led_info[id].pin);
            break;
        case LED_STATE_TOGGLE:
            gpio_bit_toggle(led_info[id].port, led_info[id].pin);
            break;
    }
}

void led_toggle(LedId id)
{
    led_set_state(id, LED_STATE_TOGGLE);
}
