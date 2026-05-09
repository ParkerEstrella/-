#include "rtc_driver.h"

static bool rtc_configured = false;

u8 bcd_to_byte(u8 bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

u8 byte_to_bcd(u8 byte)
{
    return ((byte / 10) << 4) | (byte % 10);
}

void rtc_hw_init(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);

    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();

    rtc_configured = true;
}

RetStatus rtc_set_datetime(u16 year, u8 month, u8 day,
                            u8 hour, u8 minute, u8 second)
{
    if (!rtc_configured || year < 2000 || month < 1 || month > 12 ||
        day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59) {
        return RET_INVALID_PARAM;
    }

    rtc_parameter_struct time_cfg;
    time_cfg.year        = byte_to_bcd((u8)(year - 2000));
    time_cfg.month       = byte_to_bcd(month);
    time_cfg.date        = byte_to_bcd(day);
    time_cfg.hour        = byte_to_bcd(hour);
    time_cfg.minute      = byte_to_bcd(minute);
    time_cfg.second      = byte_to_bcd(second);
    time_cfg.factor_asyn = 0x7F;
    time_cfg.factor_syn  = 0xFF;
    time_cfg.am_pm       = RTC_AM;

    rtc_init(&time_cfg);
    return RET_OK;
}

void rtc_get_timestamp(Timestamp* ts)
{
    if (ts == NULL) return;

    rtc_parameter_struct now;
    rtc_current_time_get(&now);

    ts->year   = 2000 + bcd_to_byte(now.year);
    ts->month  = bcd_to_byte(now.month);
    ts->day    = bcd_to_byte(now.date);
    ts->hour   = bcd_to_byte(now.hour);
    ts->minute = bcd_to_byte(now.minute);
    ts->second = bcd_to_byte(now.second);
}

void rtc_get_formatted(char* buf, u16 buf_size)
{
    if (buf == NULL || buf_size < 20) return;

    Timestamp ts;
    rtc_get_timestamp(&ts);
    snprintf(buf, buf_size, "%04u-%02u-%02u %02u:%02u:%02u",
             ts.year, ts.month, ts.day,
             ts.hour, ts.minute, ts.second);
}

u32 rtc_to_unix(const Timestamp* ts)
{
    if (ts == NULL) return 0;

    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_year = ts->year - 1900;
    t.tm_mon  = ts->month - 1;
    t.tm_mday = ts->day;
    t.tm_hour = ts->hour;
    t.tm_min  = ts->minute;
    t.tm_sec  = ts->second;
    t.tm_isdst = -1;

    return (u32)mktime(&t);
}
