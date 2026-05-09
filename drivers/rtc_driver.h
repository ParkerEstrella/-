#ifndef RTC_DRIVER_H
#define RTC_DRIVER_H

#include "common_types.h"

void rtc_hw_init(void);
RetStatus rtc_set_datetime(u16 year, u8 month, u8 day,
                           u8 hour, u8 minute, u8 second);
void rtc_get_timestamp(Timestamp* ts);
void rtc_get_formatted(char* buf, u16 buf_size);
u32 rtc_to_unix(const Timestamp* ts);

u8 bcd_to_byte(u8 bcd);
u8 byte_to_bcd(u8 byte);

#endif /* RTC_DRIVER_H */
