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

// =========================
// PROTOTIPOS
// =========================
static void show_profile_actions_modal(const char *code);
static void show_profile_details_modal(const char *code);
static void action_details_cb(lv_event_t *e);
static void action_duplicate_cb(lv_event_t *e);
static void show_duplicate_modal(const char *source_code);
static void duplicate_confirm_cb(lv_event_t *e);
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
// =========================
// FREE USER DATA
// =========================
static void free_code_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);
    free((void *)code);
}

// =========================
// CALLBACK DUPLICAR
// =========================
static void action_duplicate_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);

    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *modal = lv_obj_get_parent(btn);
    lv_obj_t *overlay = lv_obj_get_parent(modal);
    lv_obj_del(overlay);

    show_duplicate_modal(code);
}

static void delete_confirm_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);

    // 🔥 seguridad: no borrar perfil activo
    const char *active = active_profile_get();
    if (active && strcmp(code, active) == 0)
    {
        printf("ERROR: no podés borrar el perfil activo\n");
        return;
    }

    if (profile_delete(code))
    {
        printf("Perfil eliminado OK\n");
    }

    // cerrar modal
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *modal = lv_obj_get_parent(btn);
    lv_obj_t *overlay = lv_obj_get_parent(modal);

    lv_obj_del(overlay);

    // refrescar lista
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

        snprintf(buf, sizeof(buf), "Perfil: %s", p.code);
        lv_label_set_text(label_profile, buf);

        snprintf(buf, sizeof(buf), "Matriz: %s", p.matrix);
        lv_label_set_text(label_matrix, buf);

        snprintf(buf, sizeof(buf), "Gusano: %d", p.screw);
        lv_label_set_text(label_screw, buf);

        snprintf(buf, sizeof(buf), "Vel VFD: %d", p.vfd_speed);
        lv_label_set_text(label_vfd, buf);

        snprintf(buf, sizeof(buf), "Vel extrusión: %.2f", p.extrusion_speed);
        lv_label_set_text(label_extrusion, buf);

        snprintf(buf, sizeof(buf), "Densidad: %.2f", p.density);
        lv_label_set_text(label_density, buf);

        snprintf(buf, sizeof(buf), "Longitud: %.2f", p.cut_length);
        lv_label_set_text(label_length, buf);

        strncpy(selected_code, code, sizeof(selected_code) - 1);
        selected_code[sizeof(selected_code) - 1] = '\0';

        show_profile_actions_modal(selected_code);
    }
}

// =========================
// CONFIRMAR DUPLICADO
// =========================
static void duplicate_confirm_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *modal = lv_obj_get_parent(btn);
    lv_obj_t *overlay = lv_obj_get_parent(modal);

    // recuperar textarea
    lv_obj_t *ta = (lv_obj_t *)lv_obj_get_user_data(modal);

    const char *new_code = lv_textarea_get_text(ta);
    const char *source_code = (const char *)lv_event_get_user_data(e);

    if (strlen(new_code) == 0)
        return;

    if (profile_exists(new_code))
    {
        printf("ERROR: ya existe\n");
        return;
    }

    if (profile_duplicate(source_code, new_code))
    {
        printf("Perfil duplicado OK\n");
    }

    lv_obj_del(overlay);
    search_profiles();
}

// =========================
// MODAL DUPLICAR
// =========================
static void show_duplicate_modal(const char *source_code)
{
    lv_obj_t *overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_size(modal, 260, LV_SIZE_CONTENT);
    lv_obj_center(modal);

    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 10, 0);
    lv_obj_set_style_pad_gap(modal, 10, 0);

    lv_obj_t *title = lv_label_create(modal);
    lv_label_set_text(title, "Nuevo nombre:");

    lv_obj_t *ta = lv_textarea_create(modal);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_placeholder_text(ta, "Ej: 1234.5678.2");

    lv_obj_add_event_cb(ta, ta_duplicate_click_cb, LV_EVENT_CLICKED, NULL);

    // 🔥 guardamos textarea
    lv_obj_set_user_data(modal, ta);

    lv_obj_t *btn_ok = lv_btn_create(modal);
    lv_obj_set_width(btn_ok, LV_PCT(100));
    lv_obj_add_event_cb(btn_ok, duplicate_confirm_cb, LV_EVENT_CLICKED, (void *)source_code);

    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "Crear");
    lv_obj_center(lbl_ok);

    lv_obj_t *btn_cancel = lv_btn_create(modal);
    lv_obj_set_width(btn_cancel, LV_PCT(100));
    lv_obj_add_event_cb(btn_cancel, action_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, "Cancelar");
    lv_obj_center(lbl_cancel);
}

// =========================
// ACTIONS
// =========================
static void action_select_cb(lv_event_t *e)
{
    const char *code = (const char *)lv_event_get_user_data(e);

    active_profile_set(code);
    ui_set_active_profile(code);

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
    lv_obj_t *overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_size(modal, 250, LV_SIZE_CONTENT);
    lv_obj_center(modal);

    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 10, 0);
    lv_obj_set_style_pad_gap(modal, 10, 0);

    bool running = production_is_running();

    lv_obj_t *title = lv_label_create(modal);
    if (running)
        lv_label_set_text_fmt(title, "Perfil: %s (LOCK)", code);
    else
        lv_label_set_text_fmt(title, "Perfil: %s", code);

    lv_obj_t *btn_select = lv_btn_create(modal);
    lv_obj_set_width(btn_select, LV_PCT(100));
    lv_obj_add_event_cb(btn_select, action_select_cb, LV_EVENT_CLICKED, (void *)code);

    if (running)
        lv_obj_add_state(btn_select, LV_STATE_DISABLED);

    lv_obj_t *lbl_select = lv_label_create(btn_select);
    lv_label_set_text(lbl_select, "Seleccionar");
    lv_obj_center(lbl_select);

    lv_obj_t *btn_details = lv_btn_create(modal);
    lv_obj_set_width(btn_details, LV_PCT(100));
    lv_obj_add_event_cb(btn_details, action_details_cb, LV_EVENT_CLICKED, (void *)code);

    lv_obj_t *lbl_details = lv_label_create(btn_details);
    lv_label_set_text(lbl_details, "Detalles");
    lv_obj_center(lbl_details);

    lv_obj_t *btn_duplicate = lv_btn_create(modal);
    lv_obj_set_width(btn_duplicate, LV_PCT(100));
    lv_obj_add_event_cb(btn_duplicate, action_duplicate_cb, LV_EVENT_CLICKED, (void *)code);

    if (running)
        lv_obj_add_state(btn_duplicate, LV_STATE_DISABLED);

    lv_obj_t *lbl_dup = lv_label_create(btn_duplicate);
    lv_label_set_text(lbl_dup, "Duplicar");
    lv_obj_center(lbl_dup);



    lv_obj_t *btn_delete = lv_btn_create(modal);
    lv_obj_set_width(btn_delete, LV_PCT(100));
    lv_obj_add_event_cb(btn_delete, action_delete_cb, LV_EVENT_CLICKED, (void *)code);

    lv_obj_t *lbl_delete = lv_label_create(btn_delete);
    lv_label_set_text(lbl_delete, "Eliminar");
    lv_obj_center(lbl_delete);

    // opcional: deshabilitar si está corriendo
    if (running)
        lv_obj_add_state(btn_delete, LV_STATE_DISABLED);



    lv_obj_t *btn_cancel = lv_btn_create(modal);
    lv_obj_set_width(btn_cancel, LV_PCT(100));
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

    lv_obj_t *overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_size(modal, 280, LV_SIZE_CONTENT);
    lv_obj_center(modal);

    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 10, 0);
    lv_obj_set_style_pad_gap(modal, 8, 0);

    char buf[64];

    lv_obj_t *title = lv_label_create(modal);
    lv_label_set_text_fmt(title, "Detalles: %s", p.code);

    lv_obj_t *lbl;

    snprintf(buf, sizeof(buf), "Matriz: %s", p.matrix);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "Gusano: %d", p.screw);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "VFD: %d", p.vfd_speed);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "Extrusión: %.2f", p.extrusion_speed);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "Densidad: %.2f", p.density);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

    snprintf(buf, sizeof(buf), "Longitud: %.2f", p.cut_length);
    lbl = lv_label_create(modal);
    lv_label_set_text(lbl, buf);

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
    lv_obj_t *overlay = lv_obj_create(lv_scr_act());
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
    lv_label_set_text_fmt(label, "Eliminar perfil:\n%s ?", code);

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