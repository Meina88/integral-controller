SET(SOURCES screens/ui_Screen_1_Print.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_3d_printer_bg_png.c
    images/ui_img_btn_print_down_png.c
    images/ui_img_btn_print_top_off_png.c
    images/ui_img_icn_stop_png.c
    fonts/ui_font_Small_Font.c)

add_library(ui ${SOURCES})
