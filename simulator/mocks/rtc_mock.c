/*
 * Mock del RTC — usa la hora del sistema.
 * Reemplaza drivers/rtc/rtc.c y rtc_pcf85063a.c
 */
#include "drivers/rtc/rtc.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

void rtc_hw_init(void) {}
void rtc_set_default_time(void) {}
void rtc_set_manual_time(void) {}

void rtc_get_time_string(char *buffer)
{
    if (!buffer) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 32, "%H:%M:%S", t);
}

void rtc_get_datetime_string(char *buffer)
{
    if (!buffer) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 32, "%d/%m/%Y %H:%M:%S", t);
}

void rtc_get_date_filename_string(char *buffer)
{
    if (!buffer) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 32, "%Y%m%d_%H%M%S", t);
}

/* ── Stubs para PCF85063A (llamados directamente desde screen_config_datetime) */
void    PCF85063A_Init(void)                    {}
void    PCF85063A_Reset(void)                   {}
void    PCF85063A_Set_Time(datetime_t t)        { (void)t; }
void    PCF85063A_Set_Date(datetime_t t)        { (void)t; }
void    PCF85063A_Set_All(datetime_t t)         { (void)t; }
void    PCF85063A_Enable_Alarm(void)            {}
uint8_t PCF85063A_Get_Alarm_Flag(void)          { return 0; }
void    PCF85063A_Set_Alarm(datetime_t t)       { (void)t; }
void    PCF85063A_Read_Alarm(datetime_t *t)     { (void)t; }
void    datetime_to_str(char *s, datetime_t t)  { (void)s; (void)t; }

void PCF85063A_Read_now(datetime_t *t)
{
    if (!t) return;
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    t->year  = (uint16_t)(lt->tm_year + 1900);
    t->month = (uint8_t)(lt->tm_mon + 1);
    t->day   = (uint8_t)lt->tm_mday;
    t->hour  = (uint8_t)lt->tm_hour;
    t->min   = (uint8_t)lt->tm_min;
    t->sec   = (uint8_t)lt->tm_sec;
    t->dotw  = (uint8_t)lt->tm_wday;
}

void rtc_get_datetime(datetime_t *t)
{
    if (!t) return;
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    t->year  = (uint16_t)(lt->tm_year + 1900);
    t->month = (uint8_t)(lt->tm_mon + 1);
    t->day   = (uint8_t)lt->tm_mday;
    t->hour  = (uint8_t)lt->tm_hour;
    t->min   = (uint8_t)lt->tm_min;
    t->sec   = (uint8_t)lt->tm_sec;
    t->dotw  = (uint8_t)lt->tm_wday;
}
