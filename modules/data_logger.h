#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include "common_types.h"

/* ============================================================
 * 事件日志
 * ============================================================ */
void event_log_init(void);
void event_log_write(const char* message);
void event_log_clear_now(void);
void event_log_clear_flag_set(void);
void event_log_clear_flag_check(void);

/* ============================================================
 * 采样数据存储
 * ============================================================ */
typedef enum {
    STORE_MODE_NORMAL = 0,
    STORE_MODE_STEALTH
} StoreMode;

void datastore_init(void);
void datastore_write_sample(const Timestamp* ts, f32 real_value, StoreMode mode);
void datastore_write_overlimit(const Timestamp* ts, f32 real_value, f32 threshold);
void datastore_flush(void);

#endif /* DATA_LOGGER_H */
