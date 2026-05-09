#include "sd_card_driver.h"
#include "sdcard.h"

static FATFS g_filesystem;
static bool  g_sd_ready = false;

FATFS* sd_get_filesystem(void)
{
    return g_sd_ready ? &g_filesystem : NULL;
}

bool sd_card_is_mounted(void)
{
    return g_sd_ready;
}

RetStatus sd_card_hw_init(void)
{
    sd_error_enum sd_ret;

    sd_ret = sd_init();
    if (sd_ret != SD_OK) {
        g_sd_ready = false;
        return RET_ERR;
    }

    /* 获取卡信息 */
    sd_card_info_struct card_info;
    sd_ret = sd_card_information_get(&card_info);
    if (sd_ret != SD_OK) {
        g_sd_ready = false;
        return RET_ERR;
    }

    /* 选择卡 */
    sd_ret = sd_card_select_deselect(card_info.card_rca);
    if (sd_ret != SD_OK) {
        g_sd_ready = false;
        return RET_ERR;
    }

    /* 4-bit 总线 + DMA 传输 */
    sd_bus_mode_config(SDIO_BUSMODE_4BIT);
    sd_transfer_mode_config(SD_DMA_MODE);

    /* 挂载 FatFs */
    memset(&g_filesystem, 0, sizeof(g_filesystem));
    FRESULT res = f_mount(0, &g_filesystem);
    if (res != FR_OK) {
        g_sd_ready = false;
        return RET_ERR;
    }

    g_sd_ready = true;

    /* 创建工作目录 */
    f_mkdir("0:/sample");
    f_mkdir("0:/overLimit");
    f_mkdir("0:/log");

    return RET_OK;
}

void sd_card_get_capacity(u32* total_kb, u32* free_kb)
{
    if (!g_sd_ready) {
        if (total_kb) *total_kb = 0;
        if (free_kb)  *free_kb  = 0;
        return;
    }

    FATFS* fs = &g_filesystem;
    DWORD free_clusters;
    if (f_getfree("0:", &free_clusters, &fs) == FR_OK) {
        DWORD total = (fs->n_fatent - 2) * fs->csize;
        DWORD free  = free_clusters * fs->csize;
        if (total_kb) *total_kb = (u32)(total / 2);
        if (free_kb)  *free_kb  = (u32)(free / 2);
    }
}
