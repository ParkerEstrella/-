#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include "common_types.h"

/* LED 编号 */
typedef enum {
    LED_ID_1 = 0,
    LED_ID_2,
    LED_ID_3,
    LED_ID_4,
    LED_ID_COUNT
} LedId;

/* LED 状态 */
typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON,
    LED_STATE_TOGGLE
} LedState;

void led_initialize_all(void);
void led_set_state(LedId id, LedState state);
void led_toggle(LedId id);

#endif /* LED_DRIVER_H */
