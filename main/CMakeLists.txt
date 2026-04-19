idf_component_register(
    SRCS
        "app_main.c"
        "lvgl_port.c"
        "waveshare_rgb_lcd_port.c"

        # UI
        "ui/ui_manager.c"
        "ui/screens/screen_home.c"

        # BSP
        "bsp/display.c"
        "bsp/backlight.c"

        # DRIVERS
        "drivers/io/CH422G.c"
        "drivers/io/DEV_Config.c"
        "drivers/relay.c"
        "drivers/rtc/rtc.c"
        "drivers/rtc/rtc_pcf85063a.c"

        # STORAGE
        "storage/sdcard/sdcard.c"

        # 🔥 COMMS (AGREGADO)
        "comms/wifi/wifi_manager.c"
        "comms/http/http_server.c"

    INCLUDE_DIRS
        "."
        "ui"
        "bsp"
        "drivers"
        "drivers/io"
        "storage/sdcard"

        # 🔥 COMMS (AGREGADO)
        "comms/wifi"
        "comms/http"

        "logic"
        "services"
        "utils"

    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_http_server
        console
        fatfs
        vfs
)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)