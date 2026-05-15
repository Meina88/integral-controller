#include "screen_config.h"
#include "screen_config_datetime.h"
#include "screen_config_wifi.h"
#include "screen_config_machine.h"
#include "ui/fonts/fonts.h"
#include "lvgl.h"

#define C_BG          lv_color_hex(0x111827)
#define C_TABBAR      lv_color_hex(0x0D1117)
#define C_BORDER      lv_color_hex(0x1E293B)
#define C_TAB_ACTIVE  lv_color_hex(0x1D4ED8)
#define C_TAB_ACCENT  lv_color_hex(0x93C5FD)
#define C_TAB_MUTED   lv_color_hex(0x6B7280)
#define C_PRESSED     lv_color_hex(0x2D3748)

static lv_obj_t *root;
static lv_obj_t *content;
static lv_obj_t *screen_datetime;
static lv_obj_t *screen_wifi;
static lv_obj_t *screen_machine;
static lv_obj_t *tab_btns[3];
static lv_obj_t *tab_lbls[3];

static void switch_tab(int id)
{
    lv_obj_add_flag(screen_datetime, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen_wifi,     LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen_machine,  LV_OBJ_FLAG_HIDDEN);

    if (id == 0) lv_obj_clear_flag(screen_datetime, LV_OBJ_FLAG_HIDDEN);
    if (id == 1) lv_obj_clear_flag(screen_wifi,     LV_OBJ_FLAG_HIDDEN);
    if (id == 2) lv_obj_clear_flag(screen_machine,  LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < 3; i++)
    {
        if (i == id)
        {
            lv_obj_add_state(tab_btns[i], LV_STATE_CHECKED);
            lv_obj_set_style_text_color(tab_lbls[i], lv_color_white(), 0);
        }
        else
        {
            lv_obj_clear_state(tab_btns[i], LV_STATE_CHECKED);
            lv_obj_set_style_text_color(tab_lbls[i], C_TAB_MUTED, 0);
        }
    }
}

static void tab_cb(lv_event_t *e)
{
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    switch_tab(id);
}

lv_obj_t *screen_config_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, C_BG, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // =========================
    // TAB BAR (horizontal)
    // =========================
    lv_obj_t *tabbar = lv_obj_create(root);
    lv_obj_set_width(tabbar, LV_PCT(100));
    lv_obj_set_height(tabbar, 48);
    lv_obj_set_style_bg_color(tabbar, C_TABBAR, 0);
    lv_obj_set_style_bg_opa(tabbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(tabbar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(tabbar, C_BORDER, 0);
    lv_obj_set_style_border_width(tabbar, 1, 0);
    lv_obj_set_style_radius(tabbar, 0, 0);
    lv_obj_set_style_pad_all(tabbar, 0, 0);
    lv_obj_set_style_pad_gap(tabbar, 0, 0);
    lv_obj_set_layout(tabbar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tabbar, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(tabbar, LV_OBJ_FLAG_SCROLLABLE);

    const char *tab_names[] = {"Fecha/Hora", "WiFi", "Maquina"};

    for (int i = 0; i < 3; i++)
    {
        lv_obj_t *btn = lv_btn_create(tabbar);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_height(btn, LV_PCT(100));

        lv_obj_set_style_bg_color(btn, C_TABBAR, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);

        lv_obj_set_style_bg_color(btn, C_TAB_ACTIVE, LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 3, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(btn, C_TAB_ACCENT, LV_STATE_CHECKED);

        lv_obj_set_style_bg_color(btn, C_PRESSED, LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);

        lv_obj_add_event_cb(btn, tab_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, tab_names[i]);
        lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
        lv_obj_set_style_text_color(lbl, C_TAB_MUTED, 0);
        lv_obj_center(lbl);

        tab_btns[i] = btn;
        tab_lbls[i] = lbl;
    }

    // =========================
    // CONTENT
    // =========================
    content = lv_obj_create(root);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, C_BG, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    screen_datetime = screen_config_datetime_create(content);
    screen_wifi     = screen_config_wifi_create(content);
    screen_machine  = screen_config_machine_create(content);

    switch_tab(0);

    return root;
}

void screen_config_update(void)
{
}
