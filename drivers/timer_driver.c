#include "timer_driver.h"

static bool smp_timer_active = false;

static void timer_setup_base(u32 timer_rcu, u32 timer_periph,
                             u16 prescaler, u16 period,
                             u8 preempt_prio, u8 sub_prio, u32 irq_n)
{
    rcu_periph_clock_enable(timer_rcu);
    timer_deinit(timer_periph);

    timer_parameter_struct cfg;
    timer_struct_para_init(&cfg);
    cfg.prescaler         = prescaler;
    cfg.period            = period;
    cfg.clockdivision     = TIMER_CKDIV_DIV1;
    cfg.counterdirection  = TIMER_COUNTER_UP;
    timer_init(timer_periph, &cfg);

    timer_interrupt_enable(timer_periph, TIMER_INT_UP);
    nvic_irq_enable(irq_n, preempt_prio, sub_prio);
}

void sample_timer_setup(u16 interval_sec)
{
    /* 48000-1 预分频 => 240MHz / 48000 = 5000Hz
     * 5000 * interval_sec 周期 => interval_sec 秒触发一次 */
    u16 prescaler = 47999;  /* 0.2ms per tick */
    u16 period    = interval_sec * 5000 - 1;

    timer_setup_base(SMP_TIMER_RCU, SMP_TIMER, prescaler, period,
                     1, 1, SMP_TIMER_IRQ);
}

void sample_timer_start(void)
{
    timer_enable(SMP_TIMER);
    smp_timer_active = true;
}

void sample_timer_stop(void)
{
    timer_disable(SMP_TIMER);
    smp_timer_active = false;
}

void sample_timer_adjust_period(u16 interval_sec)
{
    timer_disable(SMP_TIMER);
    timer_autoreload_value_config(SMP_TIMER, interval_sec * 5000 - 1);
    timer_enable(SMP_TIMER);
}

bool sample_timer_is_running(void)
{
    return smp_timer_active;
}

void led_blink_timer_setup(void)
{
    /* ~1Hz 闪烁 */
    timer_setup_base(LED_TIMER_RCU, LED_TIMER, 23999, 9999,
                     1, 2, LED_TIMER_IRQ);
    timer_enable(LED_TIMER);
}
