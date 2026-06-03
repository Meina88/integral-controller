cmake_minimum_required(VERSION 3.16)
project(extrusion_simulator C)
set(CMAKE_C_STANDARD 11)

include(FetchContent)

# ── SDL2 (instalar vía MSYS2: pacman -S mingw-w64-x86_64-SDL2) ───────────────
find_package(SDL2 REQUIRED)

# ── cJSON (profile.c lo usa para parsear perfiles) ────────────────────────────
FetchContent_Declare(
    cjson
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG        v1.7.18
    GIT_SHALLOW    TRUE
)
set(ENABLE_CJSON_TEST   OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cjson)

# ── LVGL 9.3.0 ───────────────────────────────────────────────────────────────
FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.3.0
    GIT_SHALLOW    TRUE
)
set(LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LV_BUILD_DEMOS    OFF CACHE BOOL "" FORCE)

# Descargamos LVGL manualmente para poder borrar el ensamblador ARM
# antes de que cmake lo procese (set_source_files_properties no funciona
# cross-scope con FetchContent)
FetchContent_GetProperties(lvgl)
if(NOT lvgl_POPULATED)
    FetchContent_Populate(lvgl)
    # Reemplazar los archivos de ensamblador ARM con stubs vacíos.
    # No se pueden borrar porque LVGL los referencia; el ensamblador
    # x86 falla al intentar compilar instrucciones ARM.
    file(WRITE "${lvgl_SOURCE_DIR}/src/draw/sw/blend/helium/lv_blend_helium.S"
         "/* ARM Helium disabled for x86 simulator */\n")
    file(WRITE "${lvgl_SOURCE_DIR}/src/draw/sw/blend/neon/lv_blend_neon.S"
         "/* ARM NEON disabled for x86 simulator */\n")
    add_subdirectory("${lvgl_SOURCE_DIR}" "${lvgl_BINARY_DIR}")
endif()

# Configurar LVGL: apuntar a nuestro lv_conf.h y enlazarlo con SDL2
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_include_directories(lvgl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}          # lv_conf.h
    ${SDL2_INCLUDE_DIRS}                 # <SDL2/SDL.h> necesario por LV_TICK_CUSTOM
)
# SDL2 como dependencia pública de lvgl para que el linker la resuelva correctamente
target_link_libraries(lvgl PUBLIC SDL2::SDL2)

# ── Rutas del proyecto real ───────────────────────────────────────────────────
set(PROJ_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/../main")

# ── Fuentes UI (sin modificar) ────────────────────────────────────────────────
file(GLOB UI_SCREENS  "${PROJ_MAIN}/ui/screens/*.c")
file(GLOB UI_COMPS    "${PROJ_MAIN}/ui/components/*.c")
file(GLOB UI_FONTS    "${PROJ_MAIN}/ui/fonts/*.c")
set(UI_CORE
    ${PROJ_MAIN}/ui/ui_manager.c
    ${PROJ_MAIN}/ui/ui_theme.c
    ${PROJ_MAIN}/ui/ui_events.c
)

# ── Lógica (sin modificar) ────────────────────────────────────────────────────
set(LOGIC_SRCS
    ${PROJ_MAIN}/logic/alarm_config.c
    ${PROJ_MAIN}/logic/alarm.c
    ${PROJ_MAIN}/logic/calibration.c
    ${PROJ_MAIN}/logic/profile.c
    ${PROJ_MAIN}/logic/active_profile.c
    ${PROJ_MAIN}/logic/extrusion.c
    ${PROJ_MAIN}/logic/production.c
)

# ── Mocks (reemplazan drivers y servicios de hardware) ───────────────────────
file(GLOB MOCK_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/mocks/*.c")

# ── Ejecutable ────────────────────────────────────────────────────────────────
add_executable(extrusion_simulator
    sim_main.c
    ${UI_SCREENS}
    ${UI_COMPS}
    ${UI_FONTS}
    ${UI_CORE}
    ${LOGIC_SRCS}
    ${MOCK_SRCS}
)

target_include_directories(extrusion_simulator PRIVATE
    # Shims de ESP-IDF (PRIMERO para tener prioridad sobre headers del sistema)
    ${CMAKE_CURRENT_SOURCE_DIR}/include

    # Raíz de main/ — permite #include "logic/..." igual que en ESP-IDF
    ${PROJ_MAIN}

    # lv_conf.h
    ${CMAKE_CURRENT_SOURCE_DIR}

    # LVGL root
    ${lvgl_SOURCE_DIR}

    # cJSON
    ${cjson_SOURCE_DIR}
)

# En MinGW el orden es crítico: mingw32 → SDL2main → SDL2
if(MINGW)
    target_link_libraries(extrusion_simulator PRIVATE
        mingw32 SDL2::SDL2main SDL2::SDL2 lvgl cjson m)
else()
    target_link_libraries(extrusion_simulator PRIVATE
        lvgl cjson SDL2::SDL2 m)
endif()

# Suprimir warnings de redefinición de macros (DEV_Config.h vs rtc_pcf85063a.h)
target_compile_options(extrusion_simulator PRIVATE -Wno-builtin-macro-redefined)
