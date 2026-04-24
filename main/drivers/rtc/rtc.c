#include "rtc.h"
#include "rtc_pcf85063a.h"
#include <stdio.h>

static datetime_t rtc_time;

// =========================
// INIT
// =========================
void rtc_hw_init(void)
{
    PCF85063A_Init();

    // ⚠️ NO setear tiempo acá (pisaría la hora en cada arranque)
    // rtc_set_default_time();
}

// =========================
// SET DEFAULT TIME
// =========================
void rtc_set_default_time(void)
{
    rtc_time.year  = 2024;
    rtc_time.month = 1;
    rtc_time.day   = 1;
    rtc_time.dotw  = 1;

    rtc_time.hour = 12;
    rtc_time.min  = 0;
    rtc_time.sec  = 0;

    PCF85063A_Set_All(rtc_time);
}

// =========================
// SOLO HORA
// =========================
void rtc_get_time_string(char *buffer)
{
    PCF85063A_Read_now(&rtc_time);

    sprintf(buffer, "%02d:%02d:%02d",
            rtc_time.hour,
            rtc_time.min,
            rtc_time.sec);
}

// =========================
// FECHA + HORA
// =========================
void rtc_get_datetime_string(char *buffer)
{
    PCF85063A_Read_now(&rtc_time);

    snprintf(buffer, 32, "%02d-%02d-%04d %02d:%02d:%02d",
            rtc_time.day,
            rtc_time.month,
            rtc_time.year,
            rtc_time.hour,
            rtc_time.min,
            rtc_time.sec);
}

// =========================
// SET MANUAL TIME
// =========================
void rtc_set_manual_time(void)
{
    datetime_t t;

    t.year  = 2026;
    t.month = 4;
    t.day   = 24;
    t.dotw  = 5;   // jueves

    t.hour = 14;
    t.min  = 00;
    t.sec  = 0;

    PCF85063A_Set_All(t);
}