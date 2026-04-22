#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "lvgl.h"
#include "lvgl_port.h"

static const char *TAG = "lv_port";

static SemaphoreHandle_t lvgl_mux;
static TaskHandle_t lvgl_task_handle = NULL;

/* =========================
   DISPLAY FLUSH
   ========================= */
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);

    esp_lcd_panel_draw_bitmap(
        panel,
        area->x1,
        area->y1,
        area->x2 + 1,
        area->y2 + 1,
        px_map
    );
    lv_display_flush_ready(disp);
}

/* =========================
   TOUCH
   ========================= */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)lv_indev_get_user_data(indev);

    uint16_t x, y;
    uint8_t cnt = 0;

    esp_lcd_touch_read_data(tp);

    bool pressed = esp_lcd_touch_get_coordinates(tp, &x, &y, NULL, &cnt, 1);

    if (pressed && cnt > 0) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* =========================
   TICK
   ========================= */
static void tick_cb(void *arg)
{
    lv_tick_inc(LVGL_PORT_TICK_PERIOD_MS);
}

static void tick_init(void)
{
    const esp_timer_create_args_t args = {
        .callback = tick_cb,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, LVGL_PORT_TICK_PERIOD_MS * 1000));
}

/* =========================
   LVGL TASK
   ========================= */
static void lvgl_task(void *arg)
{
    while (1) {
        if (lvgl_port_lock(-1)) {
            lv_timer_handler();
            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* =========================
   INIT
   ========================= */
esp_err_t lvgl_port_init(esp_lcd_panel_handle_t lcd, esp_lcd_touch_handle_t tp)
{
    lv_init();

    tick_init();

    /* DISPLAY */
    lv_display_t *disp = lv_display_create(LVGL_PORT_H_RES, LVGL_PORT_V_RES);
    assert(disp);

    lv_display_set_user_data(disp, lcd);
    lv_display_set_flush_cb(disp, flush_cb);

    /* BUFFER */
    uint32_t buf_size = LVGL_PORT_H_RES * LVGL_PORT_BUFFER_HEIGHT;

    void *buf = heap_caps_malloc(buf_size * sizeof(lv_color_t), LVGL_PORT_BUFFER_MALLOC_CAPS);
    assert(buf);

    lv_display_set_buffers(
        disp,
        buf,
        NULL,
        buf_size * sizeof(lv_color_t),
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    /* TOUCH */
    if (tp) {
        lv_indev_t *indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, touch_read_cb);
        lv_indev_set_user_data(indev, tp);
    }

    /* MUTEX */
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);

    /* TASK */
    BaseType_t core_id = (LVGL_PORT_TASK_CORE < 0) ? tskNO_AFFINITY : LVGL_PORT_TASK_CORE;

    xTaskCreatePinnedToCore(
        lvgl_task,
        "lvgl",
        LVGL_PORT_TASK_STACK_SIZE,
        NULL,
        LVGL_PORT_TASK_PRIORITY,
        &lvgl_task_handle,
        core_id
    );

    ESP_LOGI(TAG, "LVGL 9 initialized");

    return ESP_OK;
}

/* =========================
   LOCK
   ========================= */
bool lvgl_port_lock(int timeout_ms)
{
    const TickType_t ticks = (timeout_ms < 0)
        ? portMAX_DELAY
        : pdMS_TO_TICKS(timeout_ms);

    return xSemaphoreTakeRecursive(lvgl_mux, ticks) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}

/* =========================
   VSYNC
   ========================= */
bool lvgl_port_notify_rgb_vsync(void)
{
    BaseType_t need_yield = pdFALSE;

    xTaskNotifyFromISR(lvgl_task_handle, 0, eNoAction, &need_yield);

    return need_yield == pdTRUE;
}