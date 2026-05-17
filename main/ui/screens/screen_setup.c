#include "screen_setup.h"
#include "lvgl.h"
#include "logic/profile.h"
#include "ui/components/numpad.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "logic/active_profile.h"
#include "ui/ui_manager.h"
#include "logic/production.h"

#include "ui/screens/screen_extruir.h"
#include "ui/fonts/fonts.h"
#include "ui/fonts/fa_18.h"
#include "ui/ui_theme.h"

#define C_BG (ui_theme_get()->bg)
#define C_SURFACE (ui_theme_get()->surface)
#define C_SURFACE2 (ui_theme_get()->surface2)
#define C_BORDER (ui_theme_get()->border)
#define C_PRESSED (ui_theme_get()->pressed)
#define C_MUTED (ui_theme_get()->muted)
#define C_SUBTLE (ui_theme_get()->subtle)
#define C_BLUE (ui_theme_get()->blue)
#define C_BTN_GREY (ui_theme_get()->btn_grey)

// =========================
// PROTOTIPOS
// =========================
static void show_profile_details_modal(const char *code);
static void search_profiles(void);
static void action_cancel_cb(lv_event_t *e);
static void action_select_cb(lv_event_t *e);
static void result_btn_cb(lv_event_t *e);
static void show_results(const char results[][32], int count);
static void numpad_done_cb(const char *value);

// =========================
// OBJETOS
// =========================
static lv_obj_t *root;
static lv_obj_t *ta_search;
static lv_obj_t *label_search_text;
static lv_obj_t *label_count;
static lv_obj_t *list_results;

// =========================
// HELPERS
// =========================
static lv_obj_t *make_overlay(void)
{
    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_60, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    return overlay;
}

static lv_obj_t *make_modal_panel(lv_obj_t *overlay, int w, lv_coord_t h)
{
    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_size(modal, w, h);
    lv_obj_center(modal);
    lv_obj_set_style_bg_color(modal, C_SURFACE, 0);
    lv_obj_set_style_border_color(modal, C_BORDER, 0);
    lv_obj_set_style_border_width(modal, 1, 0);
    lv_obj_set_style_radius(modal, 12, 0);
    lv_obj_set_style_clip_corner(modal, true, 0);
    lv_obj_set_style_pad_all(modal, 0, 0);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    return modal;
}

static lv_obj_t *make_modal_header(lv_obj_t *modal, const char *text)
{
    lv_obj_t *header = lv_obj_create(modal);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_height(header, 52);
    lv_obj_set_style_bg_color(header, C_SURFACE2, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_hor(header, 16, 0);
    lv_obj_set_style_pad_ver(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, text);
    lv_obj_set_style_text_font(title, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, ui_theme_get()->text, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    return header;
}

static lv_obj_t *make_modal_btn(lv_obj_t *parent, const char *text, lv_color_t bg)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_set_height(btn, 52);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn, C_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);

    return btn;
}

static void add_separator(lv_obj_t *parent)
{
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_size(sep, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep, C_BORDER, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_radius(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);
}

// =========================
// FREE USER DATA
// =========================
static void free_code_cb(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

// =========================
// RESULTADO CLICK
// =========================
static void result_btn_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);
    show_profile_details_modal(code);
}

// =========================
// ACTIONS
// =========================
static void action_select_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);

    active_profile_set(code);
    ui_set_active_profile(code);
    screen_extruir_refresh_profile();

    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *parent = lv_obj_get_parent(btn);

    while (parent)
    {
        lv_obj_t *next = lv_obj_get_parent(parent);
        if (next == lv_layer_top())
        {
            lv_obj_del(parent);
            ui_navigate_to_extruir();
            return;
        }
        parent = next;
    }
}

static void action_cancel_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *parent = lv_obj_get_parent(btn);

    while (parent)
    {
        lv_obj_t *next = lv_obj_get_parent(parent);
        if (next == lv_layer_top())
        {
            lv_obj_del(parent);
            return;
        }
        parent = next;
    }
}

// =========================
// MODAL DETALLES
// =========================
static void show_profile_details_modal(const char *code)
{
    profile_t p;
    if (!profile_get_by_code(code, &p))
        return;

    bool running = production_is_running();

    lv_obj_t *overlay = make_overlay();
    lv_obj_t *modal = make_modal_panel(overlay, 760, 455);

    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), "%s  |  %s", p.code, p.commercial_name);
    make_modal_header(modal, title_buf);

    // Body
    lv_obj_t *body = lv_obj_create(modal);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 16, 0);
    lv_obj_set_style_pad_gap(body, 16, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    // Left: data
    lv_obj_t *left = lv_obj_create(body);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_height(left, LV_PCT(100));
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_pad_all(left, 0, 0);
    lv_obj_set_style_pad_gap(left, 7, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    char buf[64];

#define ADD_ROW(key, fmt, ...)                                      \
    do                                                              \
    {                                                               \
        snprintf(buf, sizeof(buf), fmt, __VA_ARGS__);               \
        lv_obj_t *row = lv_obj_create(left);                        \
        lv_obj_set_width(row, LV_PCT(100));                         \
        lv_obj_set_height(row, LV_SIZE_CONTENT);                    \
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);             \
        lv_obj_set_style_border_width(row, 0, 0);                   \
        lv_obj_set_style_pad_all(row, 0, 0);                        \
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);                \
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);             \
        lv_obj_t *lk = lv_label_create(row);                        \
        lv_label_set_text(lk, key);                                 \
        lv_obj_set_width(lk, 155);                                  \
        lv_obj_set_style_text_font(lk, FONT_SMALL, 0);              \
        lv_obj_set_style_text_color(lk, C_MUTED, 0);                \
        lv_obj_t *lv = lv_label_create(row);                        \
        lv_label_set_text(lv, buf);                                 \
        lv_obj_set_flex_grow(lv, 1);                                \
        lv_obj_set_style_text_font(lv, FONT_SMALL, 0);              \
        lv_obj_set_style_text_color(lv, ui_theme_get()->subtle, 0); \
    } while (0)

    ADD_ROW("Matriz", "%s", p.matrix);
    ADD_ROW("Bocas", "%d", p.bocas);
    ADD_ROW("Area", "%.2f mm2", p.area_mm2);
    ADD_ROW("Gusano", "%d", p.screw);
    ADD_ROW("VFD", "%d rpm", p.vfd_rpm);
    ADD_ROW("Vel. banda", "%.2f m/min", p.belt_speed);
    ADD_ROW("Dens. teorica", "%.2f gr/m", p.theoretical_density);
    ADD_ROW("Dens. real", "%.2f gr/m", p.real_density);
    ADD_ROW("Corte default", "%.2f m", p.default_cut);

#undef ADD_ROW

    // Right: image panel
    lv_obj_t *right = lv_obj_create(body);
    lv_obj_set_size(right, 200, LV_PCT(100));
    lv_obj_set_style_bg_color(right, ui_theme_get()->surface2, 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(right, C_BORDER, 0);
    lv_obj_set_style_border_width(right, 1, 0);
    lv_obj_set_style_radius(right, 8, 0);
    lv_obj_set_style_pad_all(right, 8, 0);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    char img_path[128];
    char real_path[128];
    snprintf(img_path, sizeof(img_path), "S:/profiles/%s", p.image);
    snprintf(real_path, sizeof(real_path), "/sdcard/profiles/%s", p.image);

    printf("REAL: %s\n", real_path);
    printf("LVGL: %s\n", img_path);

    FILE *f = fopen(real_path, "r");
    if (f)
    {
        fclose(f);
        lv_obj_t *img = lv_image_create(right);
        lv_image_set_src(img, img_path);
        lv_image_set_scale(img, 307);
        lv_obj_center(img);
        lv_obj_set_style_radius(img, 6, 0);
    }
    else
    {
        lv_obj_t *no_img = lv_label_create(right);
        lv_label_set_text(no_img, "Sin imagen");
        lv_obj_set_style_text_font(no_img, FONT_SMALL, 0);
        lv_obj_set_style_text_color(no_img, C_MUTED, 0);
        lv_obj_center(no_img);
    }

    // Footer
    lv_obj_t *footer = lv_obj_create(modal);
    lv_obj_set_width(footer, LV_PCT(100));
    lv_obj_set_height(footer, 52);
    lv_obj_set_style_bg_color(footer, C_SURFACE2, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 8, 0);
    lv_obj_set_style_pad_gap(footer, 8, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_select = lv_btn_create(footer);
    lv_obj_set_flex_grow(btn_select, 1);
    lv_obj_set_height(btn_select, LV_PCT(100));
    lv_obj_set_style_bg_color(btn_select, C_BLUE, 0);
    lv_obj_set_style_bg_color(btn_select, C_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_select, 0, 0);
    lv_obj_set_style_shadow_width(btn_select, 0, 0);
    lv_obj_add_event_cb(btn_select, action_select_cb, LV_EVENT_CLICKED, (void *)code);
    if (running)
        lv_obj_add_state(btn_select, LV_STATE_DISABLED);

    lv_obj_t *lbl_select = lv_label_create(btn_select);
    lv_label_set_text(lbl_select, "Seleccionar");
    lv_obj_set_style_text_font(lbl_select, FONT_MEDIUM, 0);
    lv_obj_center(lbl_select);

    lv_obj_t *btn_close = lv_btn_create(footer);
    lv_obj_set_flex_grow(btn_close, 1);
    lv_obj_set_height(btn_close, LV_PCT(100));
    lv_obj_set_style_bg_color(btn_close, C_BTN_GREY, 0);
    lv_obj_set_style_bg_color(btn_close, C_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_close, 0, 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, action_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, "Cerrar");
    lv_obj_set_style_text_font(lbl_close, FONT_MEDIUM, 0);
    lv_obj_center(lbl_close);
}

// =========================
// RENDER RESULTADOS
// =========================
static void show_results(const char results[][32], int count)
{
    lv_obj_clean(list_results);

    if (count == 1)
        lv_label_set_text(label_count, "1 perfil encontrado");
    else
        lv_label_set_text_fmt(label_count, "%d perfiles encontrados", count);

    for (int i = 0; i < count; i++)
    {
        lv_obj_t *card = lv_obj_create(list_results);
        lv_obj_set_width(card, LV_PCT(100));
        lv_obj_set_height(card, 56);
        lv_obj_set_style_bg_color(card, C_SURFACE, 0);
        lv_obj_set_style_bg_color(card, C_PRESSED, LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_set_style_border_color(card, C_BORDER, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_pad_hor(card, 16, 0);
        lv_obj_set_style_pad_ver(card, 0, 0);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *label = lv_label_create(card);
        lv_label_set_text(label, results[i]);
        lv_obj_set_style_text_font(label, FONT_MEDIUM, 0);
        lv_obj_set_style_text_color(label, ui_theme_get()->text, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *arrow = lv_label_create(card);
        lv_label_set_text(arrow, "");
        lv_obj_set_style_text_font(arrow, &fa_18, 0);
        lv_obj_set_style_text_color(arrow, C_MUTED, 0);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, 0, 0);

        char *code_copy = malloc(32);
        if (!code_copy)
            return;
        strcpy(code_copy, results[i]);

        lv_obj_add_event_cb(card, result_btn_cb, LV_EVENT_CLICKED, code_copy);
        lv_obj_add_event_cb(card, free_code_cb, LV_EVENT_DELETE, code_copy);
    }
}

// =========================
// BUSCAR
// =========================
static void search_profiles(void)
{
    const char *txt = lv_textarea_get_text(ta_search);

    char results[10][32];
    int count = profile_search(txt, results, 10);

    show_results(results, count);
}

// =========================
// NUMPAD
// =========================
static void numpad_done_cb(const char *value)
{
    lv_textarea_set_text(ta_search, value);

    if (strlen(value) == 0)
        lv_label_set_text(label_search_text, "Toca para buscar...");
    else
        lv_label_set_text_fmt(label_search_text, "\"%s\"", value);

    search_profiles();
}

static void search_bar_click_cb(lv_event_t *e)
{
    (void)e;
    numpad_open(ta_search, numpad_done_cb);
}

// =========================
// CREATE SCREEN
// =========================
lv_obj_t *screen_setup_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, C_BG, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(root, 16, 0);
    lv_obj_set_style_pad_gap(root, 10, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // Hidden textarea: anchor for numpad, takes no layout space
    ta_search = lv_textarea_create(root);
    lv_obj_add_flag(ta_search, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_FLOATING);
    lv_textarea_set_placeholder_text(ta_search, "");

    // Search bar pill
    lv_obj_t *search_bar = lv_obj_create(root);
    lv_obj_set_width(search_bar, LV_PCT(100));
    lv_obj_set_height(search_bar, 52);
    lv_obj_set_style_bg_color(search_bar, C_SURFACE, 0);
    lv_obj_set_style_bg_opa(search_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(search_bar, C_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(search_bar, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(search_bar, C_BORDER, 0);
    lv_obj_set_style_border_width(search_bar, 1, 0);
    lv_obj_set_style_radius(search_bar, 26, 0);
    lv_obj_set_style_pad_hor(search_bar, 12, 0);
    lv_obj_set_style_pad_ver(search_bar, 0, 0);
    lv_obj_set_style_pad_gap(search_bar, 10, 0);
    lv_obj_add_flag(search_bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(search_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(search_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(search_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(search_bar, search_bar_click_cb, LV_EVENT_CLICKED, NULL);

    // Magnifier icon circle
    lv_obj_t *icon_bg = lv_obj_create(search_bar);
    lv_obj_set_size(icon_bg, 32, 32);
    lv_obj_set_style_bg_color(icon_bg, C_MUTED, 0);
    lv_obj_set_style_bg_opa(icon_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(icon_bg, 0, 0);
    lv_obj_set_style_radius(icon_bg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(icon_bg, 0, 0);
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *icon_lbl = lv_label_create(icon_bg);
    lv_label_set_text(icon_lbl, "");
    lv_obj_set_style_text_font(icon_lbl, &fa_18, 0);
    lv_obj_set_style_text_color(icon_lbl, lv_color_white(), 0);
    lv_obj_center(icon_lbl);

    // Placeholder / query text
    label_search_text = lv_label_create(search_bar);
    lv_label_set_text(label_search_text, "Toca para buscar...");
    lv_obj_set_style_text_font(label_search_text, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_search_text, C_SUBTLE, 0);
    lv_obj_set_flex_grow(label_search_text, 1);

    // Results count
    label_count = lv_label_create(root);
    lv_label_set_text(label_count, "");
    lv_obj_set_style_text_font(label_count, FONT_SMALL, 0);
    lv_obj_set_style_text_color(label_count, C_MUTED, 0);
    lv_obj_set_width(label_count, LV_PCT(100));

    // Results list
    list_results = lv_obj_create(root);
    lv_obj_set_width(list_results, LV_PCT(100));
    lv_obj_set_flex_grow(list_results, 1);
    lv_obj_set_style_bg_opa(list_results, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_results, 0, 0);
    lv_obj_set_style_pad_all(list_results, 0, 0);
    lv_obj_set_style_pad_right(list_results, 14, 0);
    lv_obj_set_style_pad_gap(list_results, 8, 0);
    lv_obj_set_layout(list_results, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_results, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_results, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    // Populate on first load
    search_profiles();

    return root;
}
