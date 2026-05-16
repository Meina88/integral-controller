idf_component_register(
    SRCS
        "app_main.c"

        # =========================
        # UI
        # =========================
        "ui/ui_manager.c"
        "ui/ui_events.c"
        "ui/ui_theme.c"

        # =========================
        # SCREENS
        # =========================
        "ui/screens/screen_extruir.c"
        "ui/screens/screen_setup.c"
        "ui/screens/screen_config.c"

        # 🔥 NUEVOS SUBMENUS CONFIG
        "ui/screens/screen_config_datetime.c"
        "ui/screens/screen_config_wifi.c"
        "ui/screens/screen_config_machine.c"
        "ui/fonts/inter_18.c"
        "ui/fonts/inter_24.c"
        "ui/fonts/inter_28.c"
        "ui/fonts/fa_18.c"        

        "ui/screens/screen_historicos.c"

        # COMPONENTS
        "ui/components/numpad.c"
        "ui/components/keyboard.c"

        # =========================
        # SERVICES
        # =========================
        "services/production_log.c"

        # =========================
        # PORT
        # =========================
        "port/lvgl_port.c"

        # =========================
        # BSP
        # =========================
        "bsp/display.c"
        "bsp/backlight.c"
        "bsp/display_rgb/waveshare_rgb_lcd_port.c"

        # =========================
        # DRIVERS
        # =========================
        "drivers/io/CH422G.c"
        "drivers/io/DEV_Config.c"
        "drivers/digital_outputs.c"
        "drivers/rtc/rtc.c"
        "drivers/rtc/rtc_pcf85063a.c"

        # =========================
        # LOGIC
        # =========================
        "logic/extrusion.c"
        "logic/profile.c"
        "logic/active_profile.c"
        "logic/production.c"
        "logic/alarm_config.c"

        # =========================
        # STORAGE
        # =========================
        "storage/nvs/storage_nvs.c"
        "storage/sdcard/sdcard.c"
        "storage/sdcard/sd_files.c"

        # =========================
        # COMMS
        # =========================
        "comms/wifi/wifi_manager.c"
        "comms/http/http_server.c"

    INCLUDE_DIRS
        "."
        "ui"
        "ui/screens"
        "ui/components"
        "ui/fonts"

        # BSP
        "bsp"
        "bsp/display_rgb"

        # DRIVERS
        "drivers"
        "drivers/io"
        "drivers/rtc"

        # STORAGE
        "storage/nvs"
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
        cjson
)

# =========================
# LVGL WARNINGS
# =========================
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
idf_component_get_property(lvgl_dir lvgl__lvgl COMPONENT_DIR)

set_property(
    TARGET ${lvgl_lib}
    APPEND PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES
    "${IDF_PATH}/components/fatfs/src"
)