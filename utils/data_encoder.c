#include "data_encoder.h"
#include "rtc_driver.h"

void encoder_timestamp_to_hex(const Timestamp* ts, f32 value, char* output, u16 buf_size)
{
    if (ts == NULL || output == NULL || buf_size < 32) return;

    u32 unix_sec = rtc_to_unix(ts);
    u16 int_part = (u16)value;
    u16 frac_part = (u16)((value - int_part) * 65536.0f);

    snprintf(output, buf_size, "%08X%04X%04X", unix_sec, int_part, frac_part);
}

void encoder_format_csv_line(const Timestamp* ts, f32 value, char* output, u16 buf_size)
{
    if (ts == NULL || output == NULL || buf_size < 64) return;

    snprintf(output, buf_size, "%04u-%02u-%02u %02u:%02u:%02u %.2fV\r\n",
             ts->year, ts->month, ts->day,
             ts->hour, ts->minute, ts->second, value);
}
