# Simulador de UI — Instrucciones desde cero

Ejecuta la UI real del proyecto en una ventana Windows sin flashear el ESP32.

---

## Requisitos previos

Tener instalado **MSYS2** desde https://msys2.org

---

## Setup inicial (solo la primera vez)

Abrir **MSYS2 MinGW64** (el ícono azul, no el gris) y ejecutar:

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 git
```

Luego, navegar a la carpeta del simulador y configurar el build (descarga LVGL y cJSON automáticamente, ~2 minutos):

```bash
cd /d/00_PROYECTOS/rubber-srl/codigo/extrusion-controller/simulator
cmake -B build -G "MinGW Makefiles"
cmake --build build -j4
./build/extrusion_simulator.exe
```

---

## Uso diario

Una vez que el setup inicial ya fue hecho, para compilar y correr:

```bash
cd /d/00_PROYECTOS/rubber-srl/codigo/extrusion-controller/simulator
cmake --build build -j4 && ./build/extrusion_simulator.exe
```

Tiempo de compilación incremental: **menos de 10 segundos**.

---

## Si algo falla o querés hacer un build limpio

```bash
cd /d/00_PROYECTOS/rubber-srl/codigo/extrusion-controller/simulator
rm -rf build
cmake -B build -G "MinGW Makefiles"
cmake --build build -j4
./build/extrusion_simulator.exe
```

---

## Control dentro del simulador

| Acción | Efecto |
|---|---|
| Clic izquierdo | Presionar botones / interactuar |
| Rueda del mouse | Scroll en listas |
| Teclado | Entrada de texto (campos WiFi/Setup) |
| Cerrar ventana | Salir del simulador |
