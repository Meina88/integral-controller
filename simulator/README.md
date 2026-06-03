# Simulador de UI — Extrusion Controller

Ejecuta la UI real del proyecto en una ventana Windows, sin necesidad de flashear el ESP32.
Compilación después del primer setup: **< 10 segundos**.

---

## Setup inicial (una sola vez)

### 1. Instalar MSYS2

Descargar e instalar desde https://msys2.org

### 2. Instalar herramientas en MSYS2 MinGW64

Abrir **MSYS2 MinGW64** (el ícono azul, no el gris) y ejecutar:

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 git
```

### 3. Primer build

```bash
# En MSYS2 MinGW64 terminal:
cd /d/00_PROYECTOS/rubber-srl/codigo/extrusion-controller/simulator

# Configurar (descarga LVGL y cJSON automáticamente, ~2 min primera vez)
cmake -B build -G "MinGW Makefiles"

# Compilar
cmake --build build -j4

# Ejecutar
./build/extrusion_simulator.exe
```

---

## Uso diario

Después del primer build, para iterar sobre cambios de UI:

```bash
cmake --build build -j4 && ./build/extrusion_simulator.exe
```

O podés agregar esta línea a un script `.sh` en la carpeta `simulator/`:

```bash
#!/bin/bash
cmake --build build -j4 && ./build/extrusion_simulator.exe
```

---

## Qué funciona

| Funcionalidad | Estado |
|---|---|
| Todas las pantallas y tabs | ✅ |
| Botones, spinners, modales | ✅ |
| Configuración (alarma, relay, calibración, spray) | ✅ |
| Botón Test del relay (con modal de cooldown) | ✅ |
| Cambio de tema claro/oscuro | ✅ |
| Valores persisten durante la sesión | ✅ |
| Valores persisten entre ejecuciones | ❌ (sin NVS) |
| Lista de perfiles | 2 perfiles de demo |
| Velocidad de extrusión | 0 (sin sensor) |
| WiFi | Siempre desconectado |

---

## Control

- **Mouse**: clic para presionar botones
- **Rueda del mouse**: scroll en listas
- **Teclado**: entrada de texto en campos (WiFi/Setup)
- **Cerrar ventana**: salir del simulador

---

## Estructura del simulador

```
simulator/
├── CMakeLists.txt      Build independiente del ESP-IDF
├── lv_conf.h           Configuración LVGL para SDL2
├── sim_main.c          Entry point (reemplaza app_main.c)
├── include/            Shims de ESP-IDF (para compilar sin ESP-IDF)
└── mocks/              Implementaciones falsas de hardware
```

Los archivos de `main/ui/`, `main/logic/`, y `main/ui/fonts/` se compilan
**sin ninguna modificación** — el simulador usa el código real del proyecto.
