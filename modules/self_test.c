#include "self_test.h"
#include "spi_flash_driver.h"
#include "sd_card_driver.h"
#include "rtc_driver.h"
#include <stdio.h>

void selftest_run_all(void)
{
    printf("\r\n==== System Self-Test ====\r\n");

    /* ---- SPI Flash 检测 ---- */
    u32 chip_id = spi_flash_read_chip_id();
    if (chip_id == FLASH_EXPECTED_ID) {
        printf("SPI Flash........OK (0x%06lX)\r\n", chip_id);
    } else {
        printf("SPI Flash........FAIL (0x%06lX, expected 0x%06lX)\r\n",
               chip_id, (u32)FLASH_EXPECTED_ID);
    }

    /* ---- TF 卡检测 ---- */
    if (!sd_card_is_mounted()) {
        printf("TF Card not mounted, attempting remount...\r\n");
        sd_card_hw_init();
    }

    if (sd_card_is_mounted()) {
        u32 total_kb, free_kb;
        sd_card_get_capacity(&total_kb, &free_kb);
        printf("TF Card..........OK (%.1f MB total)\r\n", total_kb / 1024.0f);
    } else {
        printf("TF Card..........FAIL (no card detected)\r\n");
    }

    /* ---- RTC 检测 ---- */
    Timestamp ts;
    rtc_get_timestamp(&ts);
    printf("RTC..............OK (%04u-%02u-%02u %02u:%02u:%02u)\r\n",
           ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second);

    /* ---- 团队 ID ---- */
    char team_id[TEAM_ID_LEN] = {0};
    spi_flash_read(FLASH_ADDR_TEAM_ID, (u8*)team_id, TEAM_ID_LEN - 1);
    if (team_id[0] != 0xFF && team_id[0] != '\0') {
        printf("Team ID..........%s\r\n", team_id);
    } else {
        printf("Team ID..........NOT SET\r\n");
    }

    printf("==== Self-Test Complete ====\r\n\r\n");
}
