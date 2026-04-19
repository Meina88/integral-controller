#include "lvgl.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include "drivers/relay.h"
#include "CH422G.h"
#include "drivers/rtc/rtc.h"

static bool running = false;

static int pulse_count = 0;
static int total_count = 0;

static bool last_sensor_state = false;

static lv_obj_t *label_counter;
static lv_obj_t *label_total;
static lv_obj_t *btn_label;

// 🔥 NUEVO: textarea global
static lv_obj_t *global_txt_area;

// 🔥 CONTROL RELAY
static bool relay_active = false;
static uint32_t relay_start_time = 0;

//////////////////////////////////////////////////////////
// CALLBACK ABRIR ARCHIVO
//////////////////////////////////////////////////////////
static void file_click_cb(lv_event_t *e)
{
    const char *filename = (const char *)lv_event_get_user_data(e);

    char path[128];
    snprintf(path, sizeof(path), "/sdcard/%s", filename);

    printf("Abriendo archivo: %s\n", path);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        lv_textarea_set_text(global_txt_area, "ERROR ABRIENDO ARCHIVO");
        return;
    }

    char buffer[512];
    size_t read_bytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[read_bytes] = '\0';

    fclose(f);

    lv_textarea_set_text(global_txt_area, buffer);
}

//////////////////////////////////////////////////////////
// BOTÓN START/STOP
//////////////////////////////////////////////////////////
static void start_stop_cb(lv_event_t *e)
{
    running = !running;

    char time_str[16];
    rtc_get_time_string(time_str);

    if (running)
    {
        printf("START presionado a las %s\n", time_str);

        lv_label_set_text(btn_label, "STOP");
        lv_obj_set_style_bg_color(lv_event_get_target(e),
                                  lv_palette_main(LV_PALETTE_RED), 0);
    }
    else
    {
        printf("STOP presionado a las %s\n", time_str);

        lv_label_set_text(btn_label, "START");
        lv_obj_set_style_bg_color(lv_event_get_target(e),
                                  lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

//////////////////////////////////////////////////////////
// TIMER PRINCIPAL
//////////////////////////////////////////////////////////
static void process_timer_cb(lv_timer_t *t)
{
    uint8_t di = CH422G_io_input(0x01);
    bool current_state = !di;

    if (running && current_state && !last_sensor_state)
    {
        pulse_count++;
        lv_label_set_text_fmt(label_counter, "COUNT: %d", pulse_count);

        if (pulse_count % 5 == 0)
        {
            total_count++;
            lv_label_set_text_fmt(label_total, "RELAY: %d", total_count);

            relay_set(true);
            relay_active = true;
            relay_start_time = lv_tick_get();
        }
    }

    last_sensor_state = current_state;

    if (relay_active)
    {
        if (lv_tick_get() - relay_start_time >= 1000)
        {
            relay_set(false);
            relay_active = false;
        }
    }
}

//////////////////////////////////////////////////////////
// UI
//////////////////////////////////////////////////////////
void screen_home_create(void)
{
    lv_obj_t *scr = lv_scr_act();

    // BOTÓN
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 150, 70);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 40);

    btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "START");
    lv_obj_center(btn_label);

    lv_obj_set_style_bg_color(btn,
                              lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_add_event_cb(btn, start_stop_cb, LV_EVENT_CLICKED, NULL);

    // CONTADORES
    label_counter = lv_label_create(scr);
    lv_label_set_text(label_counter, "COUNT: 0");
    lv_obj_align(label_counter, LV_ALIGN_CENTER, 0, -20);

    label_total = lv_label_create(scr);
    lv_label_set_text(label_total, "RELAY: 0");
    lv_obj_align(label_total, LV_ALIGN_CENTER, 0, 40);

    // 🔥 LISTA ARCHIVOS
    lv_obj_t *list = lv_list_create(scr);
    lv_obj_set_size(list, 400, 200);
    lv_obj_align(list, LV_ALIGN_BOTTOM_LEFT, 0, -10);

    // 🔥 TEXTAREA
    global_txt_area = lv_textarea_create(scr);
    lv_obj_set_size(global_txt_area, 400, 120);
    lv_obj_align(global_txt_area, LV_ALIGN_BOTTOM_RIGHT, 0, -10);
    lv_textarea_set_text(global_txt_area, "Selecciona archivo...");

    printf("Abriendo /sdcard...\n");

    DIR *dir = opendir("/sdcard");

    if (dir == NULL)
    {
        lv_list_add_text(list, "ERROR SD");
    }
    else
    {
        struct dirent *entry;
        bool found = false;

        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            found = true;

            printf("Archivo: %s\n", entry->d_name);

            lv_obj_t *btn = lv_list_add_btn(list, NULL, entry->d_name);

            char *name_copy = strdup(entry->d_name);
            lv_obj_add_event_cb(btn, file_click_cb, LV_EVENT_CLICKED, name_copy);
        }

        if (!found)
        {
            lv_list_add_text(list, "(SD vacia)");
        }

        closedir(dir);
    }

    lv_timer_create(process_timer_cb, 50, NULL);
}