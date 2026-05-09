#ifndef FILTER_H
#define FILTER_H

#include "common_types.h"

#define FILTER_WINDOW_SIZE 16

/* 滤波器类型 */
typedef enum {
    FILT_MOVING_AVG = 0,
    FILT_MEDIAN,
    FILT_EXPONENTIAL,
    FILT_TYPE_COUNT
} FilterType;

/* 滑动平均滤波器 */
typedef struct {
    u16 window[FILTER_WINDOW_SIZE];
    u16 index;
    u16 count;
    u32 sum;
} MovingAvgFilter;

/* 中值滤波器 */
typedef struct {
    u16 window[FILTER_WINDOW_SIZE];
    u16 index;
    u16 count;
} MedianFilter;

/* 指数滤波器 */
typedef struct {
    f32 alpha;
    f32 last_output;
    bool initialized;
} ExpFilter;

/* 通用滤波器 */
typedef struct {
    FilterType type;
    union {
        MovingAvgFilter avg;
        MedianFilter median;
        ExpFilter exp;
    } state;
} Filter;

void filter_init(Filter* filt, FilterType type);
u16 filter_process(Filter* filt, u16 input);

/* 单独的滤波器函数 */
void moving_avg_init(MovingAvgFilter* f);
u16 moving_avg_update(MovingAvgFilter* f, u16 new_val);
void median_init(MedianFilter* f);
u16 median_update(MedianFilter* f, u16 new_val);
void exp_init(ExpFilter* f, f32 alpha);
f32 exp_update(ExpFilter* f, f32 new_val);

#endif /* FILTER_H */
