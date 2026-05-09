#include "config_manager.h"
#include "spi_flash_driver.h"
#include "sd_card_driver.h"
#include <stdio.h>
#include <ctype.h>

static AppConfig g_runtime_config;

void cfgmgr_load_defaults(AppConfig* cfg)
{
    if (cfg == NULL) return;

    cfg->ratio_coeff   = 1.0f;
    cfg->limit_value   = 10.0f;
    cfg->sample_period = 5;
    cfg->filter_type   = 0;
    cfg->hide_mode     = 0;
}

void cfgmgr_save_to_flash(const AppConfig* cfg)
{
    if (cfg == NULL) return;

    printf("    ratio: %.2f\r\n", cfg->ratio_coeff);
    printf("    limit: %.2f\r\n", cfg->limit_value);

    f32 params[2];
    params[0] = cfg->ratio_coeff;
    params[1] = cfg->limit_value;

    spi_flash_erase_sector(FLASH_ADDR_CONFIG);
    spi_flash_write(FLASH_ADDR_CONFIG, (const u8*)params, sizeof(params));
    printf("Parameters saved to flash.\r\n");
}

RetStatus cfgmgr_load_from_flash(AppConfig* cfg)
{
    if (cfg == NULL) return RET_INVALID_PARAM;

    f32 params[2];
    spi_flash_read(FLASH_ADDR_CONFIG, (u8*)params, sizeof(params));

    /* 合理性检查：未初始化的 Flash 值为 0xFF，float 会是 NaN */
    if (params[0] >= 0.0f && params[0] <= 100.0f &&
        params[1] >= 0.0f && params[1] <= 200.0f) {
        cfg->ratio_coeff = params[0];
        cfg->limit_value = params[1];

        printf("    ratio: %.2f\r\n", cfg->ratio_coeff);
        printf("    limit: %.2f\r\n", cfg->limit_value);
        printf("Parameters loaded from flash.\r\n");
        return RET_OK;
    }

    printf("Flash parameters invalid, keeping current values.\r\n");
    return RET_ERR;
}

RetStatus cfgmgr_load_from_tf_card(AppConfig* cfg, const char* filepath)
{
    if (cfg == NULL || filepath == NULL) return RET_INVALID_PARAM;

    FIL fp;
    if (f_open(&fp, filepath, FA_READ) != FR_OK) {
        printf("config.ini not found.\r\n");
        return RET_NOT_FOUND;
    }

    char line[64];
    char section[16] = "";
    u8 found_ratio = 0, found_limit = 0;

    while (f_gets(line, sizeof(line), &fp)) {
        /* 去除换行符 */
        line[strcspn(line, "\r\n")] = '\0';

        /* 解析 [section] */
        char* s = strchr(line, '[');
        char* e = strchr(line, ']');
        if (s && e && e > s) {
            u16 len = (u16)(e - s - 1);
            if (len < sizeof(section)) {
                strncpy(section, s + 1, len);
                section[len] = '\0';
                for (char* p = section; *p; p++) *p = tolower(*p);
            }
            continue;
        }

        /* 解析 key=value */
        char* eq = strchr(line, '=');
        if (eq) {
            char key[32];
            u16 key_len = (u16)(eq - line);
            if (key_len >= sizeof(key)) continue;
            strncpy(key, line, key_len);
            key[key_len] = '\0';

            /* 去空格 */
            char* ks = key;
            while (*ks == ' ') ks++;
            char* ke = ks + strlen(ks) - 1;
            while (ke > ks && *ke == ' ') *ke-- = '\0';

            for (char* p = ks; *p; p++) *p = tolower(*p);

            if (strcmp(ks, "ch0") != 0 && strcmp(ks, "cho") != 0) continue;

            f32 val = (f32)atof(eq + 1);

            if (strcmp(section, "ratio") == 0) {
                cfg->ratio_coeff = val;
                found_ratio = 1;
            } else if (strcmp(section, "limit") == 0) {
                cfg->limit_value = val;
                found_limit = 1;
            }
        }
    }

    f_close(&fp);

    if (found_ratio) printf("Ratio = %.2f\r\n", cfg->ratio_coeff);
    if (found_limit) printf("Limit = %.2f\r\n", cfg->limit_value);

    if (found_ratio || found_limit) {
        printf("Config loaded from TF card.\r\n");
        return RET_OK;
    }

    printf("No valid config entries found in file.\r\n");
    return RET_ERR;
}

void cfgmgr_apply(const AppConfig* cfg)
{
    if (cfg == NULL) return;
    memcpy(&g_runtime_config, cfg, sizeof(AppConfig));
}

const AppConfig* cfgmgr_get_current(void)
{
    return &g_runtime_config;
}
