#include "screen_historicos.h"
#include "storage/sdcard/sdcard.h"

#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

static lv_obj_t *root;
static lv_obj_t *list;
static lv_obj_t *text_area;

// 🔥 estado navegación
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

    // refrescar
    lv_obj_clean(list);

    // relist
    screen_historicos_update();
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

    static char buffer[1024];
    buffer[0] = '\0';

    char line[128];

    while (fgets(line, sizeof(line), f))
    {
        strncat(buffer, line, sizeof(buffer) - strlen(buffer) - 1);
    }

    fclose(f);

    lv_textarea_set_text(text_area, buffer);
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
        // 📁 entrar carpeta
        closedir(dir);
        strcpy(current_path, path);
        screen_historicos_update();
        return;
    }

    // 📄 abrir archivo
    load_file(name);
}

// =========================
// REFRESH
// =========================
static void refresh_list(void)
{
    lv_obj_clean(list);

    DIR *dir = opendir(current_path);
    if (!dir)
        return;

    // 🔙 BACK
    if (strcmp(current_path, "/sdcard") != 0)
    {
        lv_obj_t *btn = lv_btn_create(list);
        lv_obj_set_width(btn, LV_PCT(100));

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, "..");
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

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, entry->d_name);
        lv_obj_center(label);

        char *name = strdup(entry->d_name);

        lv_obj_add_event_cb(btn, file_btn_cb, LV_EVENT_CLICKED, name);
    }

    closedir(dir);
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_historicos_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(root, 5, 0);

    // LISTA
    list = lv_obj_create(root);
    lv_obj_set_width(list, 200);
    lv_obj_set_height(list, LV_PCT(100));

    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    // VISOR
    text_area = lv_textarea_create(root);
    lv_obj_set_width(text_area, LV_PCT(100));
    lv_obj_set_height(text_area, LV_PCT(100));

    lv_textarea_set_text(text_area, "Seleccioná un archivo...");
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