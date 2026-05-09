#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ============================================================
 * 基础类型别名
 * ============================================================ */
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef float     f32;
typedef double    f64;

/* ============================================================
 * 统一返回状态码
 * ============================================================ */
typedef enum {
    RET_OK = 0,
    RET_ERR,
    RET_BUSY,
    RET_TIMEOUT,
    RET_INVALID_PARAM,
    RET_NO_MEMORY,
    RET_NOT_FOUND
} RetStatus;

/* ============================================================
 * 时间戳
 * ============================================================ */
typedef struct {
    u16 year;
    u8  month;
    u8  day;
    u8  hour;
    u8  minute;
    u8  second;
} Timestamp;

/* ============================================================
 * 采样数据结构（紧凑存储）
 * ============================================================ */
#pragma pack(1)
typedef struct {
    u32 magic;          /* 帧标识 0x44415441 */
    u32 timestamp_sec;  /* Unix 时间戳 */
    f32 raw_value;      /* ADC 原始值 */
    f32 filtered_value; /* 滤波后值 */
    f32 final_value;    /* 比例调整后最终值 */
    u8  is_over_limit;  /* 是否超限 */
} SampleRecord;
#pragma pack()

/* ============================================================
 * 应用配置
 * ============================================================ */
typedef struct {
    f32 ratio_coeff;    /* 比例系数 (0.0~100.0) */
    f32 limit_value;    /* 超限阈值 (0.0~200.0) */
    u16 sample_period;  /* 采样周期（秒） */
    u8  filter_type;    /* 滤波器类型 */
    u8  hide_mode;      /* 隐藏模式 */
} AppConfig;

/* ============================================================
 * 事件系统
 * ============================================================ */
typedef enum {
    EVT_NONE = 0,
    EVT_SAMPLE_START,
    EVT_SAMPLE_STOP,
    EVT_SAMPLE_DONE,
    EVT_KEY_PRESS,
    EVT_UART_CMD,
    EVT_TIMER_TICK,
    EVT_OVER_LIMIT,
    EVT_CONFIG_CHANGE
} EventType;

typedef struct {
    EventType type;
    u32       param1;
    u32       param2;
    void*     data_ptr;
} Event;

/* ============================================================
 * Flash 存储地址布局
 * ============================================================ */
#define FLASH_ADDR_TEAM_ID       0x000100
#define FLASH_ADDR_LOG_ID        0x020000
#define FLASH_ADDR_LOG_CACHE     0x030000
#define FLASH_ADDR_CONFIG        0x040000
#define FLASH_ADDR_CLEAR_FLAG    0x050000

#define TEAM_ID_LEN              24
#define LOG_ID_MAX               255
#define LOG_CACHE_ENTRIES        128
#define LOG_ENTRY_SIZE           128
#define FILENAME_MAX             64

/* SPI Flash 芯片特征 ID */
#define FLASH_EXPECTED_ID        0xC84013

/* ADC 参考电压 */
#define ADC_VREF                 3.3f
#define ADC_RESOLUTION           4095.0f

/* ============================================================
 * 全局时钟 tick（SysTick 驱动）
 * ============================================================ */
extern volatile u32 g_system_ticks;

/* ============================================================
 * 简易延时（毫秒）
 * ============================================================ */
static inline void delay_ms(u32 ms)
{
    u32 start = g_system_ticks;
    while ((g_system_ticks - start) < ms) {
        __NOP();
    }
}

#endif /* COMMON_TYPES_H */
