#include "key_driver.h"
#include "gd32f4xx.h"

/* 按键引脚定义 */
#define KEY1_GPIO_PORT GPIOE
#define KEY1_GPIO_PIN GPIO_PIN_3
#define KEY1_GPIO_CLK RCU_GPIOE

#define KEY2_GPIO_PORT GPIOE
#define KEY2_GPIO_PIN GPIO_PIN_4
#define KEY2_GPIO_CLK RCU_GPIOE

#define KEY3_GPIO_PORT GPIOE
#define KEY3_GPIO_PIN GPIO_PIN_5
#define KEY3_GPIO_CLK RCU_GPIOE

#define KEY4_GPIO_PORT GPIOE
#define KEY4_GPIO_PIN GPIO_PIN_6
#define KEY4_GPIO_CLK RCU_GPIOE

#define KEY5_GPIO_PORT GPIOE
#define KEY5_GPIO_PIN GPIO_PIN_7
#define KEY5_GPIO_CLK RCU_GPIOE

#define KEY6_GPIO_PORT GPIOB
#define KEY6_GPIO_PIN GPIO_PIN_0
#define KEY6_GPIO_CLK RCU_GPIOB

/* 引脚表 */
static const struct {
    uint32_t port;
    uint32_t pin;
    rcu_periph_enum clk;
} key_info[KEY_ID_COUNT] = {
    {KEY1_GPIO_PORT, KEY1_GPIO_PIN, KEY1_GPIO_CLK},
    {KEY2_GPIO_PORT, KEY2_GPIO_PIN, KEY2_GPIO_CLK},
    {KEY3_GPIO_PORT, KEY3_GPIO_PIN, KEY3_GPIO_CLK},
    {KEY4_GPIO_PORT, KEY4_GPIO_PIN, KEY4_GPIO_CLK},
    {KEY5_GPIO_PORT, KEY5_GPIO_PIN, KEY5_GPIO_CLK},
    {KEY6_GPIO_PORT, KEY6_GPIO_PIN, KEY6_GPIO_CLK},
};

/* 消抖延迟 */
#define KEY_DEBOUNCE_MS 20

void key_initialize_all(void)
{
    for (u8 i = 0; i < KEY_ID_COUNT; i++) {
        rcu_periph_clock_enable(key_info[i].clk);
        gpio_mode_set(key_info[i].port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, key_info[i].pin);
    }
}

KeyState key_get_state(KeyId id)
{
    if (id >= KEY_ID_COUNT) {
        return KEY_RELEASED;
    }
    
    if (gpio_input_bit_get(key_info[id].port, key_info[id].pin) == RESET) {
        return KEY_PRESSED;
    }
    return KEY_RELEASED;
}

KeyId key_scan_any(void)
{
    static KeyId last_pressed = KEY_ID_COUNT;
    
    for (u8 i = 0; i < KEY_ID_COUNT; i++) {
        if (key_get_state(i) == KEY_PRESSED) {
            if (last_pressed != i) {
                /* 简单消抖 */
                for (volatile u32 d = 0; d < 50000; d++) { __NOP(); }
                if (key_get_state(i) == KEY_PRESSED) {
                    last_pressed = i;
                    return i;
                }
            }
            return KEY_ID_COUNT;
        }
    }
    last_pressed = KEY_ID_COUNT;
    return KEY_ID_COUNT;
}
