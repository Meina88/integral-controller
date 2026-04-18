idf_component_register(
    SRCS
        "app_main.c"
        "lvgl_port.c"
        "waveshare_rgb_lcd_port.c"
        "ui/ui_manager.c"
        "bsp/display.c"        
        "bsp/backlight.c"
        "ui/screens/screen_home.c"
        "drivers/io/CH422G.c"
        "drivers/io/DEV_Config.c"
        "drivers/relay.c"
        "drivers/rtc/rtc.c"
        "drivers/rtc/rtc_pcf85063a.c"
        "storage/sdcard/sdcard.c"

    INCLUDE_DIRS
        "."
        "ui"
        "bsp"
        "drivers"
        "logic"
        "services"
        "utils"
        "drivers/io"
        "storage/sdcard"
)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)