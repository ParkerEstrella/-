#include "filter.h"

static void swap(u16* a, u16* b)
{
    u16 tmp = *a;
    *a = *b;
    *b = tmp;
}

/* 滑动平均滤波器 */
void moving_avg_init(MovingAvgFilter* f)
{
    if (f == NULL) return;
    f->index = 0;
    f->count = 0;
    f->sum = 0;
    for (u16 i = 0; i < FILTER_WINDOW_SIZE; i++) {
        f->window[i] = 0;
    }
}

u16 moving_avg_update(MovingAvgFilter* f, u16 new_val)
{
    if (f == NULL) return new_val;
    
    f->sum -= f->window[f->index];
    f->window[f->index] = new_val;
    f->sum += new_val;
    f->index = (f->index + 1) % FILTER_WINDOW_SIZE;
    
    if (f->count < FILTER_WINDOW_SIZE) {
        f->count++;
        return (u16)(f->sum / f->count);
    }
    return (u16)(f->sum / FILTER_WINDOW_SIZE);
}

/* 中值滤波器 */
void median_init(MedianFilter* f)
{
    if (f == NULL) return;
    f->index = 0;
    f->count = 0;
    for (u16 i = 0; i < FILTER_WINDOW_SIZE; i++) {
        f->window[i] = 0;
    }
}

u16 median_update(MedianFilter* f, u16 new_val)
{
    if (f == NULL) return new_val;
    
    f->window[f->index] = new_val;
    f->index = (f->index + 1) % FILTER_WINDOW_SIZE;
    if (f->count < FILTER_WINDOW_SIZE) {
        f->count++;
    }
    
    u16 sorted[FILTER_WINDOW_SIZE];
    for (u16 i = 0; i < f->count; i++) {
        sorted[i] = f->window[i];
    }
    
    for (u16 i = 0; i < f->count - 1; i++) {
        for (u16 j = 0; j < f->count - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                swap(&sorted[j], &sorted[j + 1]);
            }
        }
    }
    
    return sorted[f->count / 2];
}

/* 指数滤波器 */
void exp_init(ExpFilter* f, f32 alpha)
{
    if (f == NULL) return;
    f->alpha = alpha;
    f->last_output = 0.0f;
    f->initialized = false;
}

f32 exp_update(ExpFilter* f, f32 new_val)
{
    if (f == NULL) return new_val;
    
    if (!f->initialized) {
        f->last_output = new_val;
        f->initialized = true;
        return new_val;
    }
    
    f->last_output = f->alpha * new_val + (1.0f - f->alpha) * f->last_output;
    return f->last_output;
}

/* 通用滤波器接口 */
void filter_init(Filter* filt, FilterType type)
{
    if (filt == NULL) return;
    filt->type = type;
    switch (type) {
        case FILT_MOVING_AVG:
            moving_avg_init(&filt->state.avg);
            break;
        case FILT_MEDIAN:
            median_init(&filt->state.median);
            break;
        case FILT_EXPONENTIAL:
            exp_init(&filt->state.exp, 0.3f);
            break;
        default:
            break;
    }
}

u16 filter_process(Filter* filt, u16 input)
{
    if (filt == NULL) return input;
    
    switch (filt->type) {
        case FILT_MOVING_AVG:
            return moving_avg_update(&filt->state.avg, input);
        case FILT_MEDIAN:
            return median_update(&filt->state.median, input);
        case FILT_EXPONENTIAL:
            return (u16)exp_update(&filt->state.exp, (f32)input);
        default:
            return input;
    }
}
