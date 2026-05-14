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

#include <stdio.h>
#include "lvgl.h"

// =========================
// PROTOTIPOS
// =========================
static void show_profile_actions_modal(const char *code);
static void show_profile_details_modal(const char *code);
static void action_details_cb(lv_event_t *e);
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
static lv_obj_t *list_results;

// =========================
// FREE USER DATA
// =========================
static void free_code_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);
    free((void *)code);
}

// =========================
// RESULTADO CLICK
// =========================
static void result_btn_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);

    show_profile_actions_modal(code);
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
            lv_image_cache_drop(NULL);
            lv_obj_del(parent);
            return;
        }

        parent = next;
    }
}

static void action_cancel_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    // subir hasta overlay
    lv_obj_t *parent = lv_obj_get_parent(btn);

    while (parent)
    {
        lv_obj_t *next = lv_obj_get_parent(parent);

        // overlay está directamente en lv_layer_top()
        if (next == lv_layer_top())
        {
            lv_obj_del(parent);
            return;
        }

        parent = next;
    }
}

static void action_details_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);

    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *modal = lv_obj_get_parent(btn);
    lv_obj_t *overlay = lv_obj_get_parent(modal);

    lv_obj_del(overlay);

    show_profile_details_modal(code);
}

// =========================
// MODAL ACCIONES
// =========================

static void show_profile_actions_modal(const char *code)
{
    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_size(modal, 250, LV_SIZE_CONTENT);
    lv_obj_center(modal);

    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 10, 0);
    lv_obj_set_style_pad_gap(modal, 10, 0);

    bool running = production_is_running();

    // =========================
    // TITULO
    // =========================
    lv_obj_t *title = lv_label_create(modal);

    if (running)
        lv_label_set_text_fmt(title, "Perfil: %s (LOCK)", code);
    else
        lv_label_set_text_fmt(title, "Perfil: %s", code);

    // =========================
    // BOTON SELECCIONAR
    // =========================
    lv_obj_t *btn_select = lv_btn_create(modal);
    lv_obj_set_width(btn_select, LV_PCT(100));
    lv_obj_set_height(btn_select, 45);

    lv_obj_add_event_cb(btn_select, action_select_cb, LV_EVENT_CLICKED, (void *)code);

    if (running)
        lv_obj_add_state(btn_select, LV_STATE_DISABLED);

    lv_obj_t *lbl_select = lv_label_create(btn_select);
    lv_label_set_text(lbl_select, "Seleccionar");
    lv_obj_center(lbl_select);

    // =========================
    // BOTON DETALLES
    // =========================
    lv_obj_t *btn_details = lv_btn_create(modal);
    lv_obj_set_width(btn_details, LV_PCT(100));
    lv_obj_set_height(btn_details, 45);

    lv_obj_add_event_cb(btn_details, action_details_cb, LV_EVENT_CLICKED, (void *)code);

    lv_obj_t *lbl_details = lv_label_create(btn_details);
    lv_label_set_text(lbl_details, "Detalles");
    lv_obj_center(lbl_details);

    // =========================
    // BOTON CANCELAR
    // =========================
    lv_obj_t *btn_cancel = lv_btn_create(modal);
    lv_obj_set_width(btn_cancel, LV_PCT(100));
    lv_obj_set_height(btn_cancel, 45);

    lv_obj_add_event_cb(btn_cancel, action_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, "Cancelar");
    lv_obj_center(lbl_cancel);
}

// =========================
// MODAL DETALLES
// =========================
static void show_profile_details_modal(const char *code)
{
    profile_t p;

    if (!profile_get_by_code(code, &p))
        return;

    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_style_text_font(
        modal,
        FONT_SMALL,
        0);
    lv_obj_set_size(modal, 680, 360);
    lv_obj_center(modal);

    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 12, 0);
    lv_obj_set_style_pad_gap(modal, 10, 0);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);

    // =========================
    // TITULO
    // =========================
    lv_obj_t *title = lv_label_create(modal);
    lv_label_set_text_fmt(title, "Perfil: %s", p.commercial_name);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    // =========================
    // CONTENIDO HORIZONTAL
    // =========================
    lv_obj_t *content = lv_obj_create(modal);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_height(content, 240);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_pad_gap(content, 12, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // =========================
    // COLUMNA IZQUIERDA: DATOS
    // =========================
    lv_obj_t *left = lv_obj_create(content);
    lv_obj_set_size(left, 360, LV_PCT(100));
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(left, 0, 0);
    lv_obj_set_style_pad_gap(left, 5, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    char buf[64];
    lv_obj_t *lbl;

#define ADD_PARAM(fmt, ...)                           \
    do                                                \
    {                                                 \
        snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); \
        lbl = lv_label_create(left);                  \
        lv_label_set_text(lbl, buf);                  \
        lv_obj_set_width(lbl, LV_PCT(100));           \
    } while (0)

    ADD_PARAM("Matriz: %s", p.matrix);
    ADD_PARAM("Bocas: %d", p.bocas);
    ADD_PARAM("Area: %.2f mm2", p.area_mm2);
    ADD_PARAM("Gusano: %d", p.screw);
    ADD_PARAM("VFD: %d rpm", p.vfd_rpm);
    ADD_PARAM("Vel banda: %.2f m/min", p.belt_speed);
    ADD_PARAM("Densidad teorica: %.2f gr/m", p.theoretical_density);
    ADD_PARAM("Densidad real: %.2f gr/m", p.real_density);
    ADD_PARAM("Corte default: %.2f m", p.default_cut);

#undef ADD_PARAM

    // =========================
    // COLUMNA DERECHA: IMAGEN
    // =========================
    lv_obj_t *right = lv_obj_create(content);
    lv_obj_set_size(right, 210, LV_PCT(100));
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(right, 0, 0);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(right, false, 0);
    lv_obj_set_style_pad_all(right, 20, 0);

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

        // 120%
        lv_image_set_scale(img, 307);
        lv_obj_center(img);

        lv_obj_set_style_radius(img, 8, 0);
    }

    // =========================
    // BOTONES
    // =========================
    lv_obj_t *buttons = lv_obj_create(modal);
    lv_obj_set_width(buttons, LV_PCT(100));
    lv_obj_set_height(buttons, 46);
    lv_obj_set_style_border_width(buttons, 0, 0);
    lv_obj_set_style_bg_opa(buttons, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(buttons, 0, 0);
    lv_obj_set_style_pad_gap(buttons, 10, 0);
    lv_obj_set_flex_flow(buttons, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(buttons, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_select = lv_btn_create(buttons);
    lv_obj_set_flex_grow(btn_select, 1);
    lv_obj_add_event_cb(btn_select, action_select_cb, LV_EVENT_CLICKED, (void *)code);

    lv_obj_t *lbl_select = lv_label_create(btn_select);
    lv_label_set_text(lbl_select, "Seleccionar");
    lv_obj_center(lbl_select);

    lv_obj_t *btn_close = lv_btn_create(buttons);
    lv_obj_set_flex_grow(btn_close, 1);
    lv_obj_add_event_cb(btn_close, action_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, "Cerrar");
    lv_obj_center(lbl_close);
}
// =========================
// RENDER RESULTADOS
// =========================
static void show_results(const char results[][32], int count)
{
    lv_obj_clean(list_results);

    for (int i = 0; i < count; i++)
    {
        lv_obj_t *btn = lv_btn_create(list_results);
        lv_obj_set_width(btn, LV_PCT(100));

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, results[i]);
        lv_obj_center(label);

        char *code_copy = malloc(32);
        if (!code_copy)
            return;

        strcpy(code_copy, results[i]);

        lv_obj_add_event_cb(btn, result_btn_cb, LV_EVENT_CLICKED, code_copy);
        lv_obj_add_event_cb(btn, free_code_cb, LV_EVENT_DELETE, code_copy);
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
    search_profiles();
}

static void ta_click_cb(lv_event_t *e)
{
    numpad_open(ta_search, numpad_done_cb);
}

// =========================
// CREATE SCREEN
// =========================
lv_obj_t *screen_setup_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_style_text_font(root, FONT_SMALL, 0);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    lv_obj_set_style_border_width(root, 0, 0);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(root, 10, 0);
    lv_obj_set_style_pad_gap(root, 10, 0);

    ta_search = lv_textarea_create(root);
    lv_obj_set_width(ta_search, LV_PCT(100));
    lv_textarea_set_placeholder_text(ta_search, "Buscar perfil...");
    lv_obj_add_event_cb(ta_search, ta_click_cb, LV_EVENT_CLICKED, NULL);

    list_results = lv_obj_create(root);
    lv_obj_set_width(list_results, LV_PCT(100));
    lv_obj_set_flex_grow(list_results, 1);

    lv_obj_set_layout(list_results, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_results, LV_FLEX_FLOW_COLUMN);

    return root;
}