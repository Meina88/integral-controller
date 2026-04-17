idf_component_register(
    SRCS
        "app_main.c"
        "lvgl_port.c"
        "waveshare_rgb_lcd_port.c"
        "ui/ui_manager.c"
    INCLUDE_DIRS
        "."
        "ui"
        "bsp"
        "drivers"
        "logic"
        "services"
        "utils"
)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)