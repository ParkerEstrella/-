#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include "common_types.h"

#define OLED_WIDTH   128
#define OLED_HEIGHT  32
#define OLED_I2C_ADDR 0x78

void oled_hw_init(void);
void oled_clear_screen(void);
void oled_refresh(void);
void oled_show_text(u8 row, u8 col, const char* text, u8 font_size);
void oled_display_time_value(const char* time_str, const char* value_str);

#endif /* OLED_DRIVER_H */
