#include "app_main.h"
#include "adc_driver.h"
#include "uart_driver.h"
#include "key_driver.h"
#include "led_driver.h"
#include "rtc_driver.h"
#include "oled_driver.h"
#include "spi_flash_driver.h"
#include "sd_card_driver.h"
#include "timer_driver.h"
#include "command.h"
#include "filter.h"
#include "event_queue.h"
#include "config_manager.h"
#include "data_logger.h"
#include "data_encoder.h"
#include <stdio.h>
#include <string.h>

/* ============================================================
 * 全局状态
 * ============================================================ */
volatile u32 g_system_ticks = 0;
bool g_sampling_active = false;

static CmdParser    g_cmd_parser;
static Filter       g_adc_filter;
static EventQueue   g_event_queue;
static bool         g_stealth_mode = false;
static u32          g_last_display_update = 0;

/* 采样触发标志（由 TIMER5 ISR 设置，主循环消费） */
static volatile bool g_sample_triggered = false;

/* 前向声明 */
static void app_systick_config(void);
static void execute_one_sample(void);
void app_request_start_sampling(void);
void app_request_stop_sampling(void);
void app_set_stealth_mode(bool enable);
void app_change_sampling_interval(u16 sec);

/* ============================================================
 * 团队 ID 写入并校验
 * ============================================================ */
static void write_and_verify_team_id(void)
{
    const char* team_id = "2025-CIMC-20255091725";
    char wbuf[TEAM_ID_LEN];
    char rbuf[TEAM_ID_LEN];

    memset(wbuf, 0, sizeof(wbuf));
    memset(rbuf, 0, sizeof(rbuf));
    strncpy(wbuf, team_id, TEAM_ID_LEN - 1);

    spi_flash_erase_sector(FLASH_ADDR_TEAM_ID);
    spi_flash_write(FLASH_ADDR_TEAM_ID, (const u8*)wbuf, TEAM_ID_LEN);
    spi_flash_read(FLASH_ADDR_TEAM_ID, (u8*)rbuf, TEAM_ID_LEN);
    rbuf[TEAM_ID_LEN - 1] = '\0';

    printf("Device ID: %s\r\n", rbuf);
}

/* ============================================================
 * 一次性硬件初始化
 * ============================================================ */
static void hardware_bringup(void)
{
    app_systick_config();

    led_initialize_all();
    key_initialize_all();

    uart_module_init();
    spi_flash_hw_init();
    sd_card_hw_init();

    adc_module_init();
    rtc_hw_init();

    oled_hw_init();
    oled_clear_screen();
    oled_show_text(0, 0, "  Booting...", 16);
    oled_refresh();

    led_blink_timer_setup();

    cmd_parser_init(&g_cmd_parser);
    evtq_init(&g_event_queue);
    filter_init(&g_adc_filter, FILT_MOVING_AVG);

    AppConfig defaults;
    cfgmgr_load_defaults(&defaults);
    cfgmgr_apply(&defaults);

    event_log_clear_flag_check();
    event_log_init();
    datastore_init();
}

static void app_systick_config(void)
{
    if (SysTick_Config(SystemCoreClock / 1000U)) {
        while (1) {
        }
    }

    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/* ============================================================
 * 启动信息
 * ============================================================ */
static void show_banner(void)
{
    printf("\r\n");
    printf("================================\r\n");
    printf("  GD32F470 DataCollector V2.0  \r\n");
    printf("================================\r\n");

    write_and_verify_team_id();

    printf("================================\r\n");
    printf("  System Ready.\r\n");
    printf("  Type 'help' for commands.\r\n");
    printf("================================\r\n\r\n");

    event_log_write("System boot complete");
}

/* ============================================================
 * 按键分发 → 投递事件到队列
 * ============================================================ */
static void dispatch_key_event(KeyId kid)
{
    printf("KEY%d pressed\r\n", kid + 1);

    Event evt;
    evt.type     = EVT_KEY_PRESS;
    evt.param1   = (u32)kid;
    evt.param2   = 0;
    evt.data_ptr = NULL;

    evtq_put(&g_event_queue, &evt);
}

/* ---- 按键事件处理（主循环消费） ---- */
static void handle_key_event(KeyId kid)
{
    switch (kid) {
        case KEY_ID_1:
            if (g_sampling_active) {
                app_request_stop_sampling();
                event_log_write("Sampling stopped (KEY1)");
            } else {
                app_request_start_sampling();
                event_log_write("Sampling started (KEY1)");
            }
            break;

        case KEY_ID_2:
            app_change_sampling_interval(5);
            event_log_write("Sample interval -> 5s (KEY2)");
            break;

        case KEY_ID_3:
            app_change_sampling_interval(10);
            event_log_write("Sample interval -> 10s (KEY3)");
            break;

        case KEY_ID_4:
            app_change_sampling_interval(15);
            event_log_write("Sample interval -> 15s (KEY4)");
            break;

        case KEY_ID_5:
        case KEY_ID_6:
        case KEY_ID_COUNT:
            break;
    }
}

/* ============================================================
 * 单次采样（主循环中执行，不在 ISR 内）
 * ============================================================ */
static void execute_one_sample(void)
{
    u16 raw  = adc_read_single();
    u16 flt  = filter_process(&g_adc_filter, raw);
    f32 volt = adc_to_voltage(flt);
    f32 final = volt * cfgmgr_get_current()->ratio_coeff;

    Timestamp ts;
    rtc_get_timestamp(&ts);

    printf("%04u-%02u-%02u %02u:%02u:%02u ch0=%.2fV\r\n",
           ts.year, ts.month, ts.day,
           ts.hour, ts.minute, ts.second, final);

    const AppConfig* cfg = cfgmgr_get_current();

    if (g_stealth_mode) {
        datastore_write_sample(&ts, final, STORE_MODE_STEALTH);
    } else {
        if (final >= cfg->limit_value) {
            datastore_write_overlimit(&ts, final, cfg->limit_value);
            led_set_state(LED_ID_2, LED_STATE_ON);

            Event evt;
            evt.type     = EVT_OVER_LIMIT;
            evt.param1   = (u32)(final * 100.0f);
            evt.param2   = (u32)(cfg->limit_value * 100.0f);
            evt.data_ptr = NULL;
            evtq_put(&g_event_queue, &evt);
        } else {
            led_set_state(LED_ID_2, LED_STATE_OFF);
            datastore_write_sample(&ts, final, STORE_MODE_NORMAL);
        }
    }
}

/* ============================================================
 * 显示屏刷新（闲置时每秒一次）
 * ============================================================ */
static void refresh_display(void)
{
    Timestamp ts;
    rtc_get_timestamp(&ts);

    u16 raw  = adc_read_single();
    f32 volt = adc_to_voltage(raw);
    f32 final = volt * cfgmgr_get_current()->ratio_coeff;

    char time_str[20];
    char value_str[20];
    snprintf(time_str, sizeof(time_str), "%02u:%02u:%02u",
             ts.hour, ts.minute, ts.second);
    snprintf(value_str, sizeof(value_str), "%.2fV", final);

    oled_display_time_value(time_str, value_str);
}

/* ============================================================
 * 串口命令处理
 * ============================================================ */
static void service_uart_rx(void)
{
    RingBuffer* rb = uart_get_rx_buffer();
    u8 byte;

    while (ringbuf_pop(rb, &byte) == RET_OK) {
        cmd_parser_input_char(&g_cmd_parser, (char)byte);

        if (g_cmd_parser.cmd_ready) {
            const char* input = g_cmd_parser.buffer;

            if (cmd_is_waiting_for_input()) {
                cmd_process_pending_input(input);
            } else {
                cmd_execute(input);
            }

            cmd_parser_init(&g_cmd_parser);
        }
    }
}

/* ============================================================
 * 事件队列消费
 * ============================================================ */
static void process_event_queue(void)
{
    Event evt;
    while (evtq_get(&g_event_queue, &evt) == RET_OK) {
        switch (evt.type) {
            case EVT_KEY_PRESS:
                handle_key_event((KeyId)evt.param1);
                break;

            case EVT_OVER_LIMIT:
                event_log_write("Over-limit threshold exceeded");
                break;

            case EVT_SAMPLE_STOP:
                event_log_write("Sampling stopped");
                break;

            case EVT_SAMPLE_START:
                event_log_write("Sampling started");
                break;

            default:
                break;
        }
    }
}

/* ============================================================
 * 应用层 API（供 command.c 和外部调用）
 * ============================================================ */
void app_request_start_sampling(void)
{
    if (g_sampling_active) return;

    g_sampling_active = true;
    g_sample_triggered = false;
    sample_timer_setup(cfgmgr_get_current()->sample_period);
    sample_timer_start();

    printf("Sampling started (period=%us).\r\n",
           cfgmgr_get_current()->sample_period);

    oled_clear_screen();
    oled_refresh();
}

void app_request_stop_sampling(void)
{
    if (!g_sampling_active) return;

    sample_timer_stop();
    g_sampling_active = false;
    g_sample_triggered = false;

    printf("Sampling halted.\r\n");
    datastore_flush();

    oled_clear_screen();
    oled_show_text(0, 0, " System Idle", 16);
    oled_refresh();

    led_set_state(LED_ID_2, LED_STATE_OFF);
}

void app_set_stealth_mode(bool enable)
{
    g_stealth_mode = enable;
    AppConfig cfg = *cfgmgr_get_current();
    cfg.hide_mode = enable ? 1 : 0;
    cfgmgr_apply(&cfg);

    if (!enable) {
        datastore_flush();
    }
}

void app_change_sampling_interval(u16 sec)
{
    AppConfig cfg = *cfgmgr_get_current();
    cfg.sample_period = sec;
    cfgmgr_apply(&cfg);

    printf("Sample interval -> %us\r\n", sec);

    if (g_sampling_active) {
        sample_timer_adjust_period(sec);
    }
}

/* ============================================================
 * 主函数入口
 * ============================================================ */
void app_main_loop(void)
{
    hardware_bringup();
    show_banner();

    oled_clear_screen();
    oled_show_text(0, 0, " System Idle", 16);
    oled_refresh();

    while (1) {
        /* ---- 消费事件队列 ---- */
        process_event_queue();

        /* ---- 采样触发（由 TIMER5 ISR 设置标志，主循环执行） ---- */
        if (g_sample_triggered) {
            g_sample_triggered = false;
            if (g_sampling_active) {
                execute_one_sample();
            }
        }

        /* ---- 串口命令 ---- */
        service_uart_rx();

        /* ---- 按键扫描 ---- */
        if (!cmd_is_waiting_for_input()) {
            KeyId kid = key_scan_any();
            if (kid != KEY_ID_COUNT) {
                dispatch_key_event(kid);
            }
        }

        /* ---- 闲置时刷新显示屏 ---- */
        if (!g_sampling_active &&
            (g_system_ticks - g_last_display_update) >= 1000U) {
            g_last_display_update = g_system_ticks;
            refresh_display();
        }
    }
}

/* ============================================================
 * 中断服务例程（极简：只置标志、不做事）
 * ============================================================ */

void SysTick_Handler(void)
{
    g_system_ticks++;
}

void TIMER5_DAC_IRQHandler(void)
{
    if (timer_interrupt_flag_get(SMP_TIMER, TIMER_INT_FLAG_UP) != RESET) {
        timer_interrupt_flag_clear(SMP_TIMER, TIMER_INT_FLAG_UP);

        /* 只置标志，主循环会执行实际的采样和存储操作 */
        if (g_sampling_active) {
            g_sample_triggered = true;
        }
    }
}

void TIMER6_IRQHandler(void)
{
    if (timer_interrupt_flag_get(LED_TIMER, TIMER_INT_FLAG_UP) != RESET) {
        timer_interrupt_flag_clear(LED_TIMER, TIMER_INT_FLAG_UP);

        if (g_sampling_active) {
            led_toggle(LED_ID_1);
        } else {
            led_set_state(LED_ID_1, LED_STATE_OFF);
        }
    }
}
