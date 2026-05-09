#include "command.h"
#include "rtc_driver.h"
#include "spi_flash_driver.h"
#include "sd_card_driver.h"
#include "config_manager.h"
#include "data_logger.h"
#include "self_test.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ---- 外部引用（定义在 app_main.c） ---- */
extern bool g_sampling_active;
extern void app_request_start_sampling(void);
extern void app_request_stop_sampling(void);
extern void app_set_stealth_mode(bool enable);
extern void app_change_sampling_interval(u16 sec);

/* ---- 等待交互输入状态机 ---- */
typedef enum {
    INPUT_IDLE = 0,
    INPUT_WAIT_RTC,
    INPUT_WAIT_RATIO,
    INPUT_WAIT_LIMIT
} InputWaitState;

static InputWaitState s_wait_state = INPUT_IDLE;

/* ---- 预声明 ---- */
static RetStatus cmd_help(const char* args[], u8 count);
static RetStatus cmd_test(const char* args[], u8 count);
static RetStatus cmd_start(const char* args[], u8 count);
static RetStatus cmd_stop(const char* args[], u8 count);
static RetStatus cmd_rtc_set(const char* args[], u8 count);
static RetStatus cmd_rtc_now(const char* args[], u8 count);
static RetStatus cmd_ratio(const char* args[], u8 count);
static RetStatus cmd_limit(const char* args[], u8 count);
static RetStatus cmd_cfg_save(const char* args[], u8 count);
static RetStatus cmd_cfg_load(const char* args[], u8 count);
static RetStatus cmd_conf(const char* args[], u8 count);
static RetStatus cmd_hide(const char* args[], u8 count);
static RetStatus cmd_unhide(const char* args[], u8 count);
static RetStatus cmd_log_clear_now(const char* args[], u8 count);
static RetStatus cmd_log_clear_next(const char* args[], u8 count);

/* ---- 命令路由表 ---- */
const CmdEntry cmd_table[] = {
    {"help",         "Show this help message",       cmd_help,         0, 0},
    {"test",         "Run hardware self-test",       cmd_test,         0, 0},
    {"start",        "Start periodic sampling",      cmd_start,        0, 0},
    {"stop",         "Stop periodic sampling",       cmd_stop,         0, 0},
    {"RTC Config",   "Set real-time clock",          cmd_rtc_set,      0, 0},
    {"RTC now",      "Show current RTC time",        cmd_rtc_now,      0, 0},
    {"ratio",        "Show/set voltage ratio",       cmd_ratio,        0, 0},
    {"limit",        "Show/set over-limit threshold",cmd_limit,        0, 0},
    {"conf",         "Load config from TF card",     cmd_conf,         0, 0},
    {"config save",  "Persist config to SPI Flash",  cmd_cfg_save,     0, 0},
    {"config read",  "Restore config from SPI Flash",cmd_cfg_load,     0, 0},
    {"hide",         "Enable stealth storage",       cmd_hide,         0, 0},
    {"unhide",       "Disable stealth storage",      cmd_unhide,       0, 0},
    {"log clear",    "Reset log ID immediately",     cmd_log_clear_now, 0, 0},
    {"log clear next","Schedule log reset on reboot",cmd_log_clear_next,0, 0},
};

const u8 cmd_table_size = sizeof(cmd_table) / sizeof(CmdEntry);

/* ---- 等待输入状态查询 ---- */
bool cmd_is_waiting_for_input(void)
{
    return (s_wait_state != INPUT_IDLE);
}

/* ---- 处理等待中的输入 ---- */
void cmd_process_pending_input(const char* input)
{
    if (s_wait_state == INPUT_WAIT_RTC) {
        u16 yr; u8 mo, dy, hr, mi, se;
        if (sscanf(input, "%hu-%hhu-%hhu %hhu:%hhu:%hhu",
                   &yr, &mo, &dy, &hr, &mi, &se) == 6) {
            if (rtc_set_datetime(yr, mo, dy, hr, mi, se) == RET_OK) {
                printf("RTC updated -> %04u-%02u-%02u %02u:%02u:%02u\r\n",
                       yr, mo, dy, hr, mi, se);
                event_log_write("RTC time configured");
            } else {
                printf("RTC set failed: invalid values.\r\n");
            }
        } else {
            printf("Bad format. Use: YYYY-MM-DD HH:MM:SS\r\n");
        }
        s_wait_state = INPUT_IDLE;
    }
    else if (s_wait_state == INPUT_WAIT_RATIO) {
        f32 val;
        if (sscanf(input, "%f", &val) == 1 && val >= 0.0f && val <= 100.0f) {
            AppConfig cfg = *cfgmgr_get_current();
            cfg.ratio_coeff = val;
            cfgmgr_apply(&cfg);
            printf("Ratio = %.1f\r\n", val);
            event_log_write("Ratio parameter updated");
        } else {
            printf("Ratio out of range (0.0 ~ 100.0)\r\n");
        }
        s_wait_state = INPUT_IDLE;
    }
    else if (s_wait_state == INPUT_WAIT_LIMIT) {
        f32 val;
        if (sscanf(input, "%f", &val) == 1 && val >= 0.0f && val <= 200.0f) {
            AppConfig cfg = *cfgmgr_get_current();
            cfg.limit_value = val;
            cfgmgr_apply(&cfg);
            printf("Limit = %.2f\r\n", val);
            event_log_write("Limit threshold updated");
        } else {
            printf("Limit out of range (0.0 ~ 200.0)\r\n");
        }
        s_wait_state = INPUT_IDLE;
    }
}

/* ---- 命令行解析器 ---- */
void cmd_parser_init(CmdParser* parser)
{
    if (parser == NULL) return;
    parser->buf_idx   = 0;
    parser->cmd_ready = false;
    memset(parser->buffer, 0, CMD_MAX_LEN);
}

RetStatus cmd_parser_input_char(CmdParser* parser, char ch)
{
    if (parser == NULL) return RET_INVALID_PARAM;
    if (parser->cmd_ready) return RET_BUSY;

    if (ch == '\r' || ch == '\n') {
        parser->buffer[parser->buf_idx] = '\0';
        if (parser->buf_idx > 0) parser->cmd_ready = true;
        return RET_OK;
    }

    if (parser->buf_idx < CMD_MAX_LEN - 1) {
        parser->buffer[parser->buf_idx++] = ch;
    }
    return RET_OK;
}

/* 大小写不敏感前缀匹配（兼容 ARMCC 无 strncasecmp） */
static bool str_prefix_nocase(const char* str, const char* prefix)
{
    while (*prefix) {
        char a = *str++;
        char b = *prefix++;
        if (a >= 'A' && a <= 'Z') a += ('a' - 'A');
        if (b >= 'A' && b <= 'Z') b += ('a' - 'A');
        if (a != b) return false;
    }
    return true;
}

bool cmd_match(const char* input, const char* cmd_name)
{
    return str_prefix_nocase(input, cmd_name);
}

u8 cmd_split_args(char* buffer, char* args[], u8 max_args)
{
    u8 count = 0;
    char* token = strtok(buffer, " \t");
    while (token && count < max_args) {
        args[count++] = token;
        token = strtok(NULL, " \t");
    }
    return count;
}

RetStatus cmd_execute(const char* cmd_str)
{
    if (cmd_str == NULL || *cmd_str == '\0') return RET_INVALID_PARAM;

    /* 跳过前导空格 */
    while (*cmd_str == ' ' || *cmd_str == '\t') cmd_str++;
    if (*cmd_str == '\0') return RET_INVALID_PARAM;

    /* 遍历命令表，用原始输入匹配完整命令名（含空格） */
    for (u8 i = 0; i < cmd_table_size; i++) {
        u8 name_len = (u8)strlen(cmd_table[i].name);

        if (!str_prefix_nocase(cmd_str, cmd_table[i].name)) continue;

        /* 命令名之后的字符 */
        const char* rest = cmd_str + name_len;

        /* 命令名必须完整匹配（后跟空格、\0 或参数） */
        if (*rest != '\0' && *rest != ' ' && *rest != '\t') continue;

        /* 跳过命令名与参数之间的空格 */
        while (*rest == ' ' || *rest == '\t') rest++;

        /* 将剩余部分按空格拆分成参数列表 */
        char tmp[CMD_MAX_LEN];
        strncpy(tmp, rest, CMD_MAX_LEN - 1);
        tmp[CMD_MAX_LEN - 1] = '\0';

        char* args[CMD_ARG_MAX];
        u8 argc = cmd_split_args(tmp, args, CMD_ARG_MAX);

        if (argc < cmd_table[i].min_args || argc > cmd_table[i].max_args) {
            printf("Usage: %s (args: %d..%d)\r\n",
                   cmd_table[i].name, cmd_table[i].min_args, cmd_table[i].max_args);
            return RET_INVALID_PARAM;
        }

        return cmd_table[i].handler((const char**)args, argc);
    }

    printf("Unknown command: %s\r\n", cmd_str);
    return RET_ERR;
}

/* ================================================================
 * 命令处理函数实现
 * ================================================================ */

static RetStatus cmd_help(const char* args[], u8 count)
{
    (void)args; (void)count;
    printf("\r\n======== Available Commands ========\r\n");
    for (u8 i = 0; i < cmd_table_size; i++) {
        printf("  %-14s %s\r\n", cmd_table[i].name, cmd_table[i].description);
    }
    printf("====================================\r\n\r\n");
    return RET_OK;
}

static RetStatus cmd_test(const char* args[], u8 count)
{
    (void)args; (void)count;
    event_log_write("Hardware self-test triggered");
    selftest_run_all();
    return RET_OK;
}

static RetStatus cmd_start(const char* args[], u8 count)
{
    (void)args; (void)count;
    if (!g_sampling_active) {
        app_request_start_sampling();
        event_log_write("Sampling started via command");
    } else {
        printf("Already sampling.\r\n");
    }
    return RET_OK;
}

static RetStatus cmd_stop(const char* args[], u8 count)
{
    (void)args; (void)count;
    if (g_sampling_active) {
        app_request_stop_sampling();
        event_log_write("Sampling stopped via command");
    } else {
        printf("Not sampling.\r\n");
    }
    return RET_OK;
}

static RetStatus cmd_rtc_set(const char* args[], u8 count)
{
    (void)args; (void)count;
    printf("Enter datetime (YYYY-MM-DD HH:MM:SS):\r\n");
    s_wait_state = INPUT_WAIT_RTC;
    return RET_OK;
}

static RetStatus cmd_rtc_now(const char* args[], u8 count)
{
    (void)args; (void)count;
    char ts_buf[32];
    rtc_get_formatted(ts_buf, sizeof(ts_buf));
    printf("RTC: %s\r\n", ts_buf);
    return RET_OK;
}

static RetStatus cmd_ratio(const char* args[], u8 count)
{
    (void)args; (void)count;
    printf("Ratio = %.1f [0.0-100.0]. Input new value:\r\n",
           cfgmgr_get_current()->ratio_coeff);
    s_wait_state = INPUT_WAIT_RATIO;
    return RET_OK;
}

static RetStatus cmd_limit(const char* args[], u8 count)
{
    (void)args; (void)count;
    printf("Limit = %.1f [0.0-200.0]. Input new value:\r\n",
           cfgmgr_get_current()->limit_value);
    s_wait_state = INPUT_WAIT_LIMIT;
    return RET_OK;
}

static RetStatus cmd_conf(const char* args[], u8 count)
{
    (void)args; (void)count;
    AppConfig cfg = *cfgmgr_get_current();
    if (cfgmgr_load_from_tf_card(&cfg, "0:/config.ini") == RET_OK) {
        cfgmgr_apply(&cfg);
    }
    event_log_write("Config imported from TF card");
    return RET_OK;
}

static RetStatus cmd_cfg_save(const char* args[], u8 count)
{
    (void)args; (void)count;
    cfgmgr_save_to_flash(cfgmgr_get_current());
    event_log_write("Config persisted to SPI Flash");
    return RET_OK;
}

static RetStatus cmd_cfg_load(const char* args[], u8 count)
{
    (void)args; (void)count;
    AppConfig cfg = *cfgmgr_get_current();
    if (cfgmgr_load_from_flash(&cfg) == RET_OK) {
        cfgmgr_apply(&cfg);
    }
    event_log_write("Config restored from SPI Flash");
    return RET_OK;
}

static RetStatus cmd_hide(const char* args[], u8 count)
{
    (void)args; (void)count;
    app_set_stealth_mode(true);
    printf("Stealth mode: ON\r\n");
    event_log_write("Stealth storage enabled");
    return RET_OK;
}

static RetStatus cmd_unhide(const char* args[], u8 count)
{
    (void)args; (void)count;
    app_set_stealth_mode(false);
    printf("Stealth mode: OFF\r\n");
    event_log_write("Stealth storage disabled");
    return RET_OK;
}

static RetStatus cmd_log_clear_now(const char* args[], u8 count)
{
    (void)args; (void)count;
    event_log_clear_now();
    event_log_write("Log ID cleared immediately");
    return RET_OK;
}

static RetStatus cmd_log_clear_next(const char* args[], u8 count)
{
    (void)args; (void)count;
    event_log_clear_flag_set();
    event_log_write("Log clear scheduled for next boot");
    return RET_OK;
}
