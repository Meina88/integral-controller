#include "rtc.h"
#include "rtc_pcf85063a.h"
#include <stdio.h>

static datetime_t rtc_time;

// =========================
void rtc_hw_init(void)
{
    PCF85063A_Init();
    rtc_set_default_time();
}

// =========================
void rtc_set_default_time(void)
{
    rtc_time.year = 24;
    rtc_time.month = 1;
    rtc_time.day = 1;    

    rtc_time.hour = 12;
    rtc_time.min = 0;
    rtc_time.sec = 0;

    PCF85063A_Set_All(rtc_time);  // 🔥 sin &
}

// =========================
void rtc_get_time_string(char *buffer)
{
    PCF85063A_Read_now(&rtc_time);

    sprintf(buffer, "%02d:%02d:%02d",
            rtc_time.hour,
            rtc_time.min,
            rtc_time.sec);
}