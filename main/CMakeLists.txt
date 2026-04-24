idf_component_register(
    SRCS
        "app_main.c"

        # UI (TU CAPA)
        "ui/ui_manager.c"
        "ui/ui_events.c"

        # 🔥 SCREENS
        "ui/screens/screen_extruir.c"
        "ui/screens/screen_setup.c"
        "ui/screens/screen_config.c"

        # PORT
        "port/lvgl_port.c"

        # BSP
        "bsp/display.c"
        "bsp/backlight.c"
        "bsp/display_rgb/waveshare_rgb_lcd_port.c"

        # DRIVERS
        "drivers/io/CH422G.c"
        "drivers/io/DEV_Config.c"
        "drivers/digital_outputs.c"
        "drivers/rtc/rtc.c"
        "drivers/rtc/rtc_pcf85063a.c"

        # LOGIC
        "logic/extrusion.c"

        # STORAGE
        "storage/sdcard/sdcard.c"
        "storage/sdcard/sd_files.c"

        # COMMS
        "comms/wifi/wifi_manager.c"
        "comms/http/http_server.c"

    INCLUDE_DIRS
        "."
        "ui"
        "ui/screens"   # 🔥 IMPORTANTE

        # BSP
        "bsp"
        "bsp/display_rgb"

        # DRIVERS
        "drivers"
        "drivers/io"
        "drivers/rtc"

        # STORAGE
        "storage/sdcard"

        # COMMS
        "comms/wifi"
        "comms/http"

        # CORE
        "logic"
        "services"
        "utils"
        "port"

    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_http_server
        console
        fatfs
        vfs
        esp_timer
        lvgl
)

# Opcional: silenciar warnings de LVGL
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)