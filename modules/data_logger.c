#include "data_logger.h"
#include "sd_card_driver.h"
#include "spi_flash_driver.h"
#include "rtc_driver.h"
#include "data_encoder.h"
#include "config_manager.h"
#include <stdio.h>

/* ============================================================
 * 事件日志系统
 * ============================================================ */
static FIL   g_log_file;
static u8    g_log_seq_id = 0;
static bool  g_log_file_ready = false;
static u16   g_log_cache_written = 0;

void event_log_init(void)
{
    FATFS* fs = sd_get_filesystem();
    if (fs == NULL) {
        g_log_file_ready = false;
        return;
    }

    /* 读取上次的 log ID */
    u8 saved_id;
    spi_flash_read(FLASH_ADDR_LOG_ID, &saved_id, sizeof(saved_id));
    g_log_seq_id = (saved_id == 0xFF || saved_id > LOG_ID_MAX) ? 0 : saved_id;

    /* 检查 SPI Flash 中是否有未导出的缓存日志 */
    bool has_cached = false;
    for (u16 i = 0; i < LOG_CACHE_ENTRIES && !has_cached; i++) {
        u8 first_byte;
        spi_flash_read(FLASH_ADDR_LOG_CACHE + i * LOG_ENTRY_SIZE, &first_byte, 1);
        if (first_byte != 0xFF) has_cached = true;
    }

    /* 导出缓存日志到 TF 卡 */
    if (has_cached) {
        f_mkdir("0:/log");
        char fname[32];
        snprintf(fname, sizeof(fname), "0:/log/log%d.txt", g_log_seq_id - 1);
        FIL cache_fp;
        if (f_open(&cache_fp, fname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
            for (u16 i = 0; i < LOG_CACHE_ENTRIES; i++) {
                char entry[LOG_ENTRY_SIZE];
                spi_flash_read(FLASH_ADDR_LOG_CACHE + i * LOG_ENTRY_SIZE,
                               (u8*)entry, LOG_ENTRY_SIZE);
                if (entry[0] == 0xFF) break;
                UINT bw;
                f_write(&cache_fp, entry, strlen(entry), &bw);
            }
            f_close(&cache_fp);
        }
        /* 清除缓存 */
        spi_flash_erase_sector(FLASH_ADDR_LOG_CACHE);
        g_log_cache_written = 0;
    }

    /* 递增 log ID */
    u8 next_id = g_log_seq_id + 1;
    if (next_id > LOG_ID_MAX) next_id = 0;
    spi_flash_erase_sector(FLASH_ADDR_LOG_ID);
    spi_flash_write(FLASH_ADDR_LOG_ID, &next_id, sizeof(next_id));

    /* 创建新日志文件 */
    char fname[32];
    snprintf(fname, sizeof(fname), "0:/log/log%d.txt", g_log_seq_id);
    if (f_open(&g_log_file, fname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        g_log_file_ready = true;
    }
}

void event_log_write(const char* message)
{
    if (message == NULL) return;

    char line[LOG_ENTRY_SIZE];
    Timestamp ts;
    rtc_get_timestamp(&ts);

    snprintf(line, sizeof(line), "%04u-%02u-%02u %02u:%02u:%02u %s\r\n",
             ts.year, ts.month, ts.day,
             ts.hour, ts.minute, ts.second, message);

    if (g_log_file_ready) {
        UINT bw;
        f_write(&g_log_file, line, strlen(line), &bw);
        f_sync(&g_log_file);
    } else {
        /* 缓存到 SPI Flash */
        if (g_log_cache_written < LOG_CACHE_ENTRIES) {
            u32 addr = FLASH_ADDR_LOG_CACHE + g_log_cache_written * LOG_ENTRY_SIZE;
            spi_flash_write(addr, (const u8*)line, LOG_ENTRY_SIZE);
            g_log_cache_written++;
        }
    }
}

void event_log_clear_now(void)
{
    g_log_seq_id = 0;
    spi_flash_erase_sector(FLASH_ADDR_LOG_ID);
    spi_flash_write(FLASH_ADDR_LOG_ID, &g_log_seq_id, sizeof(g_log_seq_id));
    printf("Log ID reset to 0.\r\n");
}

void event_log_clear_flag_set(void)
{
    u8 flag = 0xAA;
    spi_flash_erase_sector(FLASH_ADDR_CLEAR_FLAG);
    spi_flash_write(FLASH_ADDR_CLEAR_FLAG, &flag, sizeof(flag));
    printf("Next power-on will clear log ID.\r\n");
}

void event_log_clear_flag_check(void)
{
    u8 flag;
    spi_flash_read(FLASH_ADDR_CLEAR_FLAG, &flag, sizeof(flag));

    if (flag == 0xAA) {
        printf("Auto-clearing log ID as scheduled...\r\n");
        spi_flash_erase_sector(FLASH_ADDR_LOG_ID);
        u8 zero = 0;
        spi_flash_write(FLASH_ADDR_LOG_ID, &zero, sizeof(zero));

        flag = 0xFF;
        spi_flash_erase_sector(FLASH_ADDR_CLEAR_FLAG);
        spi_flash_write(FLASH_ADDR_CLEAR_FLAG, &flag, sizeof(flag));
        printf("Log ID cleared.\r\n");
    }
}

/* ============================================================
 * 采样数据文件存储
 * ============================================================ */
static FIL   g_sample_fp;
static bool  g_sample_fp_open = false;
static u8    g_sample_line_count = 0;

static FIL   g_overlimit_fp;
static bool  g_overlimit_fp_open = false;
static u8    g_overlimit_line_count = 0;

static FIL   g_stealth_fp;
static bool  g_stealth_fp_open = false;
static u8    g_stealth_line_count = 0;

/* ---- 内部辅助 ---- */
static void make_timestamped_fname(char* buf, u16 size,
                                   const char* dir, const char* prefix)
{
    Timestamp ts;
    rtc_get_timestamp(&ts);
    snprintf(buf, size, "0:/%s/%s%04u%02u%02u%02u%02u%02u.txt",
             dir, prefix, ts.year, ts.month, ts.day,
             ts.hour, ts.minute, ts.second);
}

static void rotate_file(FIL* fp, bool* is_open, u8* counter,
                        const char* dir, const char* prefix)
{
    if (*is_open) {
        f_close(fp);
        *is_open = false;
    }

    char fname[FILENAME_MAX];
    make_timestamped_fname(fname, sizeof(fname), dir, prefix);
    f_mkdir(dir);
    if (f_open(fp, fname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        *is_open = true;
        *counter = 0;
        printf("New file: %s\r\n", fname);
    }
}

/* ---- 公共接口 ---- */
void datastore_init(void)
{
    g_sample_fp_open     = false;
    g_overlimit_fp_open  = false;
    g_stealth_fp_open    = false;
}

void datastore_write_sample(const Timestamp* ts, f32 real_value, StoreMode mode)
{
    if (ts == NULL) return;

    if (mode == STORE_MODE_STEALTH) {
        /* 隐藏模式：写入 hideData 目录 */
        if (!g_stealth_fp_open || g_stealth_line_count >= 10) {
            if (g_stealth_fp_open) {
                f_close(&g_stealth_fp);
                g_stealth_fp_open = false;
            }

            char fname[FILENAME_MAX];
            make_timestamped_fname(fname, sizeof(fname), "hideData", "hideData");
            f_mkdir("0:/hideData");
            if (f_open(&g_stealth_fp, fname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
                g_stealth_fp_open = true;
                g_stealth_line_count = 0;
                printf("New hideData: %s\r\n", fname);
            }
        }

        if (g_stealth_fp_open) {
            char line[128];
            encoder_format_csv_line(ts, real_value, line, sizeof(line));
            UINT bw;
            f_write(&g_stealth_fp, line, strlen(line), &bw);

            /* 追加加密行 */
            char enc[32];
            encoder_timestamp_to_hex(ts, real_value, enc, sizeof(enc));
            char enc_line[48];
            snprintf(enc_line, sizeof(enc_line), "hide: %s\r\n", enc);
            f_write(&g_stealth_fp, enc_line, strlen(enc_line), &bw);

            g_stealth_line_count++;
            printf("hide: %s%s\r\n", enc, (real_value > cfgmgr_get_current()->limit_value) ? "*" : "");
        }
        return;
    }

    /* 正常模式 */
    if (real_value < cfgmgr_get_current()->limit_value) {
        if (!g_sample_fp_open || g_sample_line_count >= 10) {
            rotate_file(&g_sample_fp, &g_sample_fp_open, &g_sample_line_count,
                        "sample", "sampleData");
        }

        if (g_sample_fp_open) {
            char line[128];
            encoder_format_csv_line(ts, real_value, line, sizeof(line));
            UINT bw;
            f_write(&g_sample_fp, line, strlen(line), &bw);
            g_sample_line_count++;
        }
    }
}

void datastore_write_overlimit(const Timestamp* ts, f32 real_value, f32 threshold)
{
    if (ts == NULL) return;

    if (!g_overlimit_fp_open || g_overlimit_line_count >= 10) {
        rotate_file(&g_overlimit_fp, &g_overlimit_fp_open,
                    &g_overlimit_line_count, "overLimit", "overLimit");
    }

    if (g_overlimit_fp_open) {
        char line[128];
        encoder_format_csv_line(ts, real_value, line, sizeof(line));
        UINT bw;
        f_write(&g_overlimit_fp, line, strlen(line), &bw);
        g_overlimit_line_count++;

        if (!cfgmgr_get_current()->hide_mode) {
            printf("%04u-%02u-%02u %02u:%02u:%02u ch0=%.2fV OverLimit(%.2fV)!\r\n",
                   ts->year, ts->month, ts->day,
                   ts->hour, ts->minute, ts->second,
                   real_value, threshold);
        }
    }
}

void datastore_flush(void)
{
    if (g_sample_fp_open)    { f_close(&g_sample_fp);    g_sample_fp_open    = false; }
    if (g_overlimit_fp_open) { f_close(&g_overlimit_fp); g_overlimit_fp_open = false; }
    if (g_stealth_fp_open)   { f_close(&g_stealth_fp);   g_stealth_fp_open   = false; }
}
