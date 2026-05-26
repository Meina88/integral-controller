#include "screen_config_system.h"

#include "comms/ota/ota_manager.h"
#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"

#include "lvgl.h"
#include <stdio.h>
#include <string.h>

#define C_BG      (ui_theme_get()->bg)
#define C_SURFACE (ui_theme_get()->surface)
#define C_BORDER  (ui_theme_get()->border)
#define C_MUTED   (ui_theme_get()->muted)
#define C_SUBTLE  (ui_theme_get()->subtle)
#define C_TEXT    (ui_theme_get()->text)
#define C_BLUE    (ui_theme_get()->blue)
#define C_GREEN   (ui_theme_get()->green)
#define C_RED     (ui_theme_get()->red)
#define C_PRESSED (ui_theme_get()->pressed)

static lv_obj_t *root;
static lv_obj_t *label_current_version;
static lv_obj_t *label_available_version;
static lv_obj_t *label_state;
static lv_obj_t *row_progress;
static lv_obj_t *label_progress;
static lv_obj_t *label_error;
static lv_obj_t *btn_update;
static lv_obj_t *label_btn_update;

static const char *state_to_text(ota_state_t state)
{
    switch (state) {
    case OTA_STATE_IDLE:        return "En espera";
    case OTA_STATE_CHECKING:    return "Buscando actualizacion...";
    case OTA_STATE_DOWNLOADING: return "Descargando firmware...";
    case OTA_STATE_SUCCESS:     return "Actualizacion aplicada";
    case OTA_STATE_ERROR:       return "Error";
    default:                    return "Desconocido";
    }
}

static void ota_check_cb(lv_event_t *e)
{
    (void)e;
    ota_check_for_update();
}

static void ota_update_cb(lv_event_t *e)
{
    (void)e;
    ota_start_update();
}

static lv_obj_t *make_row(lv_obj_t *parent, const char *title, lv_obj_t **value_lbl)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 10, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = lv_label_create(row);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_width(lbl_title, 180);
    lv_obj_set_style_text_font(lbl_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_title, C_SUBTLE, 0);

    lv_obj_t *lbl_val = lv_label_create(row);
    lv_label_set_text(lbl_val, "-");
    lv_obj_set_flex_grow(lbl_val, 1);
    lv_obj_set_style_text_font(lbl_val, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_val, C_TEXT, 0);

    *value_lbl = lbl_val;
    return row;
}

lv_obj_t *screen_config_system_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, C_BG, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_pad_all(root, 20, 0);
    lv_obj_set_style_pad_gap(root, 12, 0);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "Sistema");
    lv_obj_set_style_text_font(title, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, C_TEXT, 0);

    lv_obj_t *subtitle = lv_label_create(root);
    lv_label_set_text(subtitle, "Gestion de firmware OTA");
    lv_obj_set_style_text_font(subtitle, FONT_SMALL, 0);
    lv_obj_set_style_text_color(subtitle, C_MUTED, 0);

    lv_obj_t *card = lv_obj_create(root);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, C_SURFACE, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, C_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_gap(card, 10, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    make_row(card, "Version instalada", &label_current_version);
    make_row(card, "Ultima disponible", &label_available_version);
    make_row(card, "Estado OTA", &label_state);
    row_progress = make_row(card, "Progreso", &label_progress);
    lv_obj_add_flag(row_progress, LV_OBJ_FLAG_HIDDEN);

    label_error = lv_label_create(card);
    lv_label_set_text(label_error, "");
    lv_obj_set_style_text_font(label_error, FONT_SMALL, 0);
    lv_obj_set_style_text_color(label_error, C_RED, 0);
    lv_obj_set_width(label_error, LV_PCT(100));

    lv_obj_t *btn_row = lv_obj_create(root);
    lv_obj_set_width(btn_row, LV_PCT(100));
    lv_obj_set_height(btn_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_pad_gap(btn_row, 10, 0);
    lv_obj_set_layout(btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_check = lv_btn_create(btn_row);
    lv_obj_set_size(btn_check, 180, 44);
    lv_obj_set_style_bg_color(btn_check, C_SURFACE, 0);
    lv_obj_set_style_bg_opa(btn_check, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn_check, C_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_check, C_BORDER, 0);
    lv_obj_set_style_border_width(btn_check, 1, 0);
    lv_obj_set_style_shadow_width(btn_check, 0, 0);
    lv_obj_set_style_radius(btn_check, 8, 0);
    lv_obj_add_event_cb(btn_check, ota_check_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_check = lv_label_create(btn_check);
    lv_label_set_text(lbl_check, "Buscar actualizacion");
    lv_obj_set_style_text_font(lbl_check, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_check, C_TEXT, 0);
    lv_obj_center(lbl_check);

    btn_update = lv_btn_create(btn_row);
    lv_obj_set_size(btn_update, 240, 44);
    lv_obj_set_style_bg_color(btn_update, C_BLUE, 0);
    lv_obj_set_style_bg_opa(btn_update, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn_update, C_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_update, 0, 0);
    lv_obj_set_style_shadow_width(btn_update, 0, 0);
    lv_obj_set_style_radius(btn_update, 8, 0);
    lv_obj_add_event_cb(btn_update, ota_update_cb, LV_EVENT_CLICKED, NULL);

    label_btn_update = lv_label_create(btn_update);
    lv_label_set_text(label_btn_update, "Actualizar firmware");
    lv_obj_set_style_text_font(label_btn_update, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_btn_update, lv_color_white(), 0);
    lv_obj_center(label_btn_update);

    return root;
}

void screen_config_system_update(void)
{
    if (!root || !lv_obj_is_valid(root))
        return;

    ota_status_t st;
    ota_get_status(&st);

    lv_label_set_text(label_current_version, st.current_version[0] ? st.current_version : "-");
    lv_label_set_text(label_available_version, st.available_version[0] ? st.available_version : "-");
    lv_label_set_text(label_state, state_to_text(st.state));

    char progress_buf[32];
    snprintf(progress_buf, sizeof(progress_buf), "%d%%", st.progress);
    lv_label_set_text(label_progress, progress_buf);

    if (st.state == OTA_STATE_ERROR && st.last_error[0]) {
        lv_label_set_text_fmt(label_error, "Detalle: %s", st.last_error);
    } else {
        lv_label_set_text(label_error, "");
    }

    bool show_progress = (st.state == OTA_STATE_DOWNLOADING);
    if (show_progress)
        lv_obj_clear_flag(row_progress, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(row_progress, LV_OBJ_FLAG_HIDDEN);

    bool update_in_progress = (st.state == OTA_STATE_DOWNLOADING || st.state == OTA_STATE_CHECKING || st.busy);
    bool can_update = st.update_available && !update_in_progress;

    if (update_in_progress) {
        lv_label_set_text(label_btn_update, "Actualizando...");
        lv_obj_add_state(btn_update, LV_STATE_DISABLED);
    } else if (can_update) {
        lv_label_set_text(label_btn_update, "Actualizar firmware");
        lv_obj_clear_state(btn_update, LV_STATE_DISABLED);
    } else {
        lv_label_set_text(label_btn_update, "Sistema actualizado");
        lv_obj_add_state(btn_update, LV_STATE_DISABLED);
    }

    if (st.state == OTA_STATE_SUCCESS) {
        lv_obj_set_style_text_color(label_state, C_GREEN, 0);
    } else if (st.state == OTA_STATE_ERROR) {
        lv_obj_set_style_text_color(label_state, C_RED, 0);
    } else {
        lv_obj_set_style_text_color(label_state, C_TEXT, 0);
    }
}
