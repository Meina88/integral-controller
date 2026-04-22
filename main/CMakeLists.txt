idf_component_register(
    SRCS
        "app_main.c"

        # UI CORE
        "ui/ui_manager.c"
        "ui/ui_events.c"

        # SQUARELINE UI
        "ui/squareline/ui.c"
        "ui/squareline/ui_helpers.c"
        "ui/squareline/components/ui_comp_hook.c"

        # SQUARELINE SCREENS
        "ui/squareline/screens/ui_Screen_1_Print.c"

        # SQUARELINE ASSETS
        "ui/squareline/fonts/ui_font_Small_Font.c"

        "ui/squareline/images/ui_img_3d_printer_bg_png.c"
        "ui/squareline/images/ui_img_btn_print_down_png.c"
        "ui/squareline/images/ui_img_btn_print_top_off_png.c"
        "ui/squareline/images/ui_img_icn_stop_png.c"

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
        "ui/squareline"
        "ui/squareline/components"
        "ui/squareline/screens"
        "ui/squareline/fonts"
        "ui/squareline/images"
        "ui/squareline/.."
        "bsp"
        "bsp/display_rgb"

        "drivers"
        "drivers/io"

        "storage/sdcard"

        "comms/wifi"
        "comms/http"

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
)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)