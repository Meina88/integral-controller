# Extrusion Controller — instrucciones para Claude

## Commits

- Siempre en inglés
- Máximo dos líneas
- Formato listo para copiar y pegar:
  ```
  git commit -m "type(scope): short description"
  ```

## Stack del proyecto

- **MCU**: ESP32-S3 con ESP-IDF 5.5.x (CMake)
- **UI**: LVGL 9.3.0, display RGB 800×480, touch GT911
- **Fuentes**: inter_18/24/28 compiladas como arrays C con formato comprimido (`LV_USE_FONT_COMPRESSED`)
- **Almacenamiento**: NVS (Flash) vía capa de abstracción `storage/nvs/storage_nvs.c`
- **I/O digital**: IO expander CH422G (I2C) — no GPIO directo

## Arquitectura de capas (de arriba hacia abajo)

```
UI screens (main/ui/screens/)
    ↓ llaman a
Logic layer (main/logic/)          ← sin ESP-IDF directo
    ↓ llama a
Driver abstractions (main/drivers/) ← wrappean hardware
    ↓
Hardware (GPIO, I2C, RTC, relays)
```

Las pantallas **nunca** llaman a drivers directamente. La lógica **nunca** llama a NVS directamente.

## Convenciones de código

- Sin comentarios salvo que el WHY sea no obvio
- Sin docstrings ni bloques de comentarios multilínea
- Fuentes: `FONT_SMALL` = inter_18, `FONT_MEDIUM` = inter_24, `FONT_LARGE` = inter_28
- Símbolos Unicode literales: usar `"▲"` / `"▼"`, no `LV_SYMBOL_UP/DOWN` (FontAwesome no incluido)
- Objetos LVGL que son overlays o se referencian desde callbacks: null-out en `LV_EVENT_DELETE` + guard con `lv_obj_is_valid()` antes de usar

## Simulador de UI para PC (simulator/)

El proyecto incluye un simulador que compila la UI real para Windows sin flashear el ESP32.

### Setup (una sola vez)

Requiere MSYS2 con entorno **MinGW64**:
```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 git
```

### Build y ejecución

```bash
cd simulator
rm -rf build
cmake -B build -G "MinGW Makefiles" -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j4
./build/extrusion_simulator.exe
```

Para iteraciones diarias (sin limpiar):
```bash
cmake --build build -j4 && ./build/extrusion_simulator.exe
```

### Cómo funciona

- `simulator/CMakeLists.txt` descarga LVGL 9.3.0 y cJSON via FetchContent
- Los archivos `.S` de ARM (Helium/NEON) se reemplazan con stubs vacíos antes de compilar
- `simulator/mocks/` contiene implementaciones falsas de todos los drivers de hardware
- `simulator/include/` contiene shims mínimos de ESP-IDF para que el código compile en x86
- Los archivos de `main/ui/` y `main/logic/` se compilan **sin modificar**

### Problemas conocidos al crear un simulador para proyectos LVGL 9.x en MinGW64

Estas son las soluciones a todos los errores encontrados, en orden:

1. **Ejemplos internos de LVGL fallan** → habilitar todos los widgets en `lv_conf.h` (`LV_USE_KEYBOARD`, `LV_USE_WIN`, `LV_USE_CANVAS`, etc.)
2. **ARM assembly (Helium/NEON)** → reemplazar con stubs vacíos en `FetchContent_Populate` antes de `add_subdirectory`
3. **`SDL_main` conflicto de firma** → usar `int main(int argc, char *argv[])` en el entry point
4. **SDL2 undefined references en lvgl** → `target_link_libraries(lvgl PUBLIC SDL2::SDL2)`
5. **Orden de linkeo MinGW** → `mingw32 SDL2::SDL2main SDL2::SDL2 lvgl cjson m`
6. **Fuentes comprimidas crash** → `#define LV_USE_FONT_COMPRESSED 1` en `lv_conf.h`
7. **`CONFIG_I2C_MASTER_SCL` undefined** → crear `simulator/include/sdkconfig.h` con los valores

### Qué simula y qué no

| Funcionalidad | Estado |
|---|---|
| Todas las pantallas y navegación | ✅ |
| Configuración (alarm, relay, calibración, spray) | ✅ |
| Cambio de tema claro/oscuro | ✅ |
| Persistencia de valores (durante la sesión) | ✅ |
| Persistencia entre ejecuciones | ❌ |
| Perfiles (2 de demo precargados) | ✅ parcial |
| Velocidad de extrusión real | ❌ (siempre 0) |
| WiFi | ❌ (siempre desconectado) |
