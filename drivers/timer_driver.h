#ifndef TIMER_DRIVER_H
#define TIMER_DRIVER_H

#include "common_types.h"

/* 采样定时器 -> TIMER5 */
#define SMP_TIMER          TIMER5
#define SMP_TIMER_RCU      RCU_TIMER5
#define SMP_TIMER_IRQ      TIMER5_DAC_IRQn
#define SMP_TIMER_IRQH     TIMER5_DAC_IRQHandler

/* LED 闪烁定时器 -> TIMER6 */
#define LED_TIMER          TIMER6
#define LED_TIMER_RCU      RCU_TIMER6
#define LED_TIMER_IRQ      TIMER6_IRQn

void sample_timer_setup(u16 interval_sec);
void sample_timer_start(void);
void sample_timer_stop(void);
void sample_timer_adjust_period(u16 interval_sec);
bool sample_timer_is_running(void);

void led_blink_timer_setup(void);

#endif /* TIMER_DRIVER_H */
