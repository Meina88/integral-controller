#include "screen_historicos.h"
#include "storage/sdcard/sdcard.h"

#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"

static lv_obj_t *root;
static lv_obj_t *list;
static lv_obj_t *text_area;

static char current_path[128] = "/sdcard";

// =========================
// BACK
// =========================
static void back_btn_cb(lv_event_t *e)
{
    char *last = strrchr(current_path, '/');

    if (last && last != current_path)
        *last = '\0';
    else
        strcpy(current_path, "/sdcard");

    lv_obj_clean(list);
    screen_historicos_update();
}

static void free_name_cb(lv_event_t *e)
{
    char *name = lv_event_get_user_data(e);
    free(name);
}

// =========================
// MOSTRAR ARCHIVO
// =========================
static void load_file(const char *filename)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", current_path, filename);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        lv_textarea_set_text(text_area, "Error abriendo archivo");
        return;
    }

    static char buffer[2048];

    fseek(f, 0, SEEK_END);
    long size = ftell(f);

    long start = size - (long)(sizeof(buffer) - 1);
    if (start < 0)
        start = 0;

    fseek(f, start, SEEK_SET);

    size_t read_bytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[read_bytes] = '\0';

    fclose(f);

    lv_textarea_set_text(text_area, buffer);
    lv_textarea_set_cursor_pos(text_area, LV_TEXTAREA_CURSOR_LAST);
}

// =========================
// CLICK ITEM
// =========================
static void file_btn_cb(lv_event_t *e)
{
    char *name = (char *)lv_event_get_user_data(e);

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", current_path, name);

    DIR *dir = opendir(path);

    if (dir)
    {
        closedir(dir);
        strcpy(current_path, path);
        screen_historicos_update();
        return;
    }

    load_file(name);
}

// =========================
// REFRESH
// =========================
static void refresh_list(void)
{
    const ui_theme_t *th = ui_theme_get();
    lv_obj_clean(list);

    DIR *dir = opendir(current_path);
    if (!dir)
        return;

    if (strcmp(current_path, "/sdcard") != 0)
    {
        lv_obj_t *btn = lv_btn_create(list);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_set_height(btn, 40);
        lv_obj_set_style_bg_color(btn, th->surface, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(btn, th->pressed, LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, th->border, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, "..");
        lv_obj_set_style_text_font(label, FONT_SMALL, 0);
        lv_obj_set_style_text_color(label, th->muted, 0);
        lv_obj_center(label);

        lv_obj_add_event_cb(btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        lv_obj_t *btn = lv_btn_create(list);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_set_height(btn, 40);
        lv_obj_set_style_bg_color(btn, th->surface, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(btn, th->pressed, LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, th->border, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, entry->d_name);
        lv_obj_set_style_text_font(label, FONT_SMALL, 0);
        lv_obj_set_style_text_color(label, th->text, 0);
        lv_obj_center(label);

        char *name = strdup(entry->d_name);

        lv_obj_add_event_cb(btn, free_name_cb, LV_EVENT_DELETE, name);
        lv_obj_add_event_cb(btn, file_btn_cb,  LV_EVENT_CLICKED, name);
    }

    closedir(dir);
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_historicos_create(lv_obj_t *parent)
{
    const ui_theme_t *th = ui_theme_get();

    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, th->bg, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_pad_all(root, 8, 0);
    lv_obj_set_style_pad_gap(root, 8, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // LISTA
    list = lv_obj_create(root);
    lv_obj_set_width(list, 200);
    lv_obj_set_height(list, LV_PCT(100));
    lv_obj_set_style_bg_color(list, th->surface, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(list, th->border, 0);
    lv_obj_set_style_border_width(list, 1, 0);
    lv_obj_set_style_radius(list, 8, 0);
    lv_obj_set_style_pad_all(list, 6, 0);
    lv_obj_set_style_pad_gap(list, 4, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    // VISOR
    text_area = lv_textarea_create(root);
    lv_obj_set_width(text_area, LV_PCT(100));
    lv_obj_set_height(text_area, LV_PCT(100));
    lv_obj_set_flex_grow(text_area, 1);
    lv_obj_set_style_bg_color(text_area, th->surface, 0);
    lv_obj_set_style_bg_opa(text_area, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(text_area, th->border, 0);
    lv_obj_set_style_border_width(text_area, 1, 0);
    lv_obj_set_style_radius(text_area, 8, 0);
    lv_obj_set_style_text_font(text_area, FONT_SMALL, 0);
    lv_obj_set_style_text_color(text_area, th->text, 0);

    lv_textarea_set_text(text_area, "Selecciona un archivo...");
    lv_textarea_set_cursor_click_pos(text_area, false);
    lv_obj_clear_flag(text_area, LV_OBJ_FLAG_CLICKABLE);

    refresh_list();

    return root;
}

// =========================
// UPDATE
// =========================
void screen_historicos_update(void)
{
    refresh_list();
}
