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

static void action_delete_cb(lv_event_t *e);
static void show_delete_confirm_modal(const char *code);
static void delete_confirm_cb(lv_event_t *e);

static void action_delete_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);

    // cerrar modal actual
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *modal = lv_obj_get_parent(btn);
    lv_obj_t *overlay = lv_obj_get_parent(modal);
    lv_obj_del(overlay);

    // abrir confirmación
    show_delete_confirm_modal(code);
}

// =========================
// OBJETOS
// =========================
static lv_obj_t *root;
static lv_obj_t *ta_search;
static lv_obj_t *list_results;

static lv_obj_t *label_profile;
static lv_obj_t *label_matrix;
static lv_obj_t *label_screw;
static lv_obj_t *label_vfd;
static lv_obj_t *label_extrusion;
static lv_obj_t *label_density;
static lv_obj_t *label_length;

static char selected_code[32] = "";

// =========================
// TECLADO
// =========================

static void ta_duplicate_click_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);

    // abrir numpad pasando el textarea
    numpad_open(ta, NULL);
}

static void show_error_modal(const char *msg)
{
    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_size(modal, 260, LV_SIZE_CONTENT);
    lv_obj_center(modal);

    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 10, 0);
    lv_obj_set_style_pad_gap(modal, 10, 0);

    // 🔴 mensaje
    lv_obj_t *label = lv_label_create(modal);
    lv_label_set_text(label, msg);

    // botón OK
    lv_obj_t *btn = lv_btn_create(modal);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_add_event_cb(btn, action_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "OK");
    lv_obj_center(lbl);
}

// =========================
// FREE USER DATA
// =========================
static void free_code_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);
    free((void *)code);
}

static void delete_confirm_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);

    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *modal = lv_obj_get_parent(btn);
    lv_obj_t *overlay = lv_obj_get_parent(modal);

    const char *active = active_profile_get();

    // 🔴 CASO: perfil activo
    if (active && strcmp(code, active) == 0)
    {
        printf("ERROR: no podés borrar el perfil activo\n");

        // 🔥 1. cerrar modal actual SIEMPRE
        lv_obj_del(overlay);

        // 🔥 2. mostrar error arriba
        show_error_modal("No podés borrar el perfil activo");

        return;
    }

    // 🔴 CASO: error general
    if (!profile_delete(code))
    {
        lv_obj_del(overlay);
        show_error_modal("Error al eliminar perfil");
        return;
    }

    // ✅ OK
    lv_obj_del(overlay);
    search_profiles();
}

// =========================
// RESULTADO CLICK
// =========================
static void result_btn_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);

    profile_t p;

    if (profile_get_by_code(code, &p))
    {
        char buf[64];

        // =========================
        // MOSTRAR DATOS
        // =========================
        snprintf(buf, sizeof(buf), "Perfil: %s", p.commercial_name);
        lv_label_set_text(label_profile, buf);

        snprintf(buf, sizeof(buf), "Matriz: %s", p.matrix);
        lv_label_set_text(label_matrix, buf);

        snprintf(buf, sizeof(buf), "Bocas: %d", p.bocas);
        lv_label_set_text(label_screw, buf);

        snprintf(buf, sizeof(buf), "VFD: %d rpm", p.vfd_rpm);
        lv_label_set_text(label_vfd, buf);

        snprintf(buf, sizeof(buf), "Vel: %.2f m/min", p.belt_speed);
        lv_label_set_text(label_extrusion, buf);

        snprintf(buf, sizeof(buf), "Densidad: %.2f gr/m", p.theoretical_density);
        lv_label_set_text(label_density, buf);

        snprintf(buf, sizeof(buf), "Corte: %.2f m", p.default_cut);
        lv_label_set_text(label_length, buf);

        // =========================
        // 🔥 NUEVO: ABRIR MODAL
        // =========================
        show_profile_actions_modal(code);
    }
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
    lv_obj_t *modal = lv_obj_get_parent(btn);
    lv_obj_t *overlay = lv_obj_get_parent(modal);

    lv_obj_del(overlay);
}

static void action_cancel_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *modal = lv_obj_get_parent(btn);
    lv_obj_t *overlay = lv_obj_get_parent(modal);

    lv_obj_del(overlay);
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
    lv_obj_set_size(modal, 280, LV_SIZE_CONTENT);
    lv_obj_center(modal);

    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 10, 0);
    lv_obj_set_style_pad_gap(modal, 8, 0);

    char buf[64];
    lv_obj_t *lbl;

    // =========================
    // TITULO
    // =========================
    lv_obj_t *title = lv_label_create(modal);
    lv_label_set_text_fmt(title, "Perfil: %s", p.commercial_name);

    // =========================
    // GEOMETRÍA
    // =========================
    snprintf(buf, sizeof(buf), "Matriz: %s", p.matrix);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "Bocas: %d", p.bocas);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "Área: %.2f mm²", p.area_mm2);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    // =========================
    // PROCESO
    // =========================
    snprintf(buf, sizeof(buf), "Gusano: %d", p.screw);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "VFD: %d rpm", p.vfd_rpm);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "Vel banda: %.2f m/min", p.belt_speed);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    // =========================
    // INGENIERÍA
    // =========================
    snprintf(buf, sizeof(buf), "Densidad teórica: %.2f gr/m", p.theoretical_density);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "Densidad real: %.2f gr/m", p.real_density);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    // =========================
    // PRODUCCIÓN
    // =========================
    snprintf(buf, sizeof(buf), "Corte default: %.2f m", p.default_cut);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    // =========================
    // BOTÓN CERRAR
    // =========================
    lv_obj_t *btn_close = lv_btn_create(modal);
    lv_obj_set_width(btn_close, LV_PCT(100));
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

static void show_delete_confirm_modal(const char *code)
{
    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_size(modal, 260, LV_SIZE_CONTENT);
    lv_obj_center(modal);

    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 10, 0);
    lv_obj_set_style_pad_gap(modal, 10, 0);

    // texto
    lv_obj_t *label = lv_label_create(modal);

    lv_label_set_recolor(label, true);

    lv_label_set_text_fmt(label,
                          "Eliminar perfil:\n%s\n\n#ff0000 ⚠ IRREVERSIBLE #",
                          code);

    // botón confirmar
    lv_obj_t *btn_ok = lv_btn_create(modal);
    lv_obj_set_width(btn_ok, LV_PCT(100));
    lv_obj_add_event_cb(btn_ok, delete_confirm_cb, LV_EVENT_CLICKED, (void *)code);

    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "Eliminar");
    lv_obj_center(lbl_ok);

    // botón cancelar
    lv_obj_t *btn_cancel = lv_btn_create(modal);
    lv_obj_set_width(btn_cancel, LV_PCT(100));
    lv_obj_add_event_cb(btn_cancel, action_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, "Cancelar");
    lv_obj_center(lbl_cancel);
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

    label_profile = lv_label_create(root);
    lv_label_set_text(label_profile, "Perfil:");

    label_matrix = lv_label_create(root);
    lv_label_set_text(label_matrix, "Matriz:");

    label_screw = lv_label_create(root);
    lv_label_set_text(label_screw, "Gusano:");

    label_vfd = lv_label_create(root);
    lv_label_set_text(label_vfd, "Vel VFD:");

    label_extrusion = lv_label_create(root);
    lv_label_set_text(label_extrusion, "Vel extrusión:");

    label_density = lv_label_create(root);
    lv_label_set_text(label_density, "Densidad:");

    label_length = lv_label_create(root);
    lv_label_set_text(label_length, "Longitud:");

    return root;
}