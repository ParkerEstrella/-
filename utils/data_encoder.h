#ifndef DATA_ENCODER_H
#define DATA_ENCODER_H

#include "common_types.h"

void encoder_timestamp_to_hex(const Timestamp* ts, f32 value, char* output, u16 buf_size);
void encoder_format_csv_line(const Timestamp* ts, f32 value, char* output, u16 buf_size);

#endif /* DATA_ENCODER_H */
