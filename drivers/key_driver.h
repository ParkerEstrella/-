#ifndef KEY_DRIVER_H
#define KEY_DRIVER_H

#include "common_types.h"

/* 按键编号 */
typedef enum {
    KEY_ID_1 = 0,
    KEY_ID_2,
    KEY_ID_3,
    KEY_ID_4,
    KEY_ID_5,
    KEY_ID_6,
    KEY_ID_COUNT
} KeyId;

/* 按键状态 */
typedef enum {
    KEY_RELEASED = 0,
    KEY_PRESSED
} KeyState;

void key_initialize_all(void);
KeyState key_get_state(KeyId id);
KeyId key_scan_any(void);

#endif /* KEY_DRIVER_H */
