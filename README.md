# PandaTouch Bluetooth StreamDeck

Transforma tu **PandaTouch (ESP32-S3)** en un potente **StreamDeck Bluetooth nativo (HID)**. No requiere drivers ni software adicional en el PC, funciona como un teclado Bluetooth estándar.

![PandaTouch](docs/images/pandatouch.png)

## ✨ Características (Fork mejorado)

- ⌨️ **Bluetooth HID Nativo**: El dispositivo se identifica como un teclado Bluetooth. Sin latencia y sin necesidad de WiFi.
- 🍏🪟 **Soporte Dual Nativo (macOS y Windows)**: Perfiles independientes y automáticos para ambos sistemas. Tecla CMD/GUI adaptada.
- 🚀 **Escritura Ultra-Rápida (Modo Turbo)**: Lanzamiento de apps en menos de 1 segundo (Typing delay reducido a 5ms).
- 📂 **Almacenamiento LittleFS**: Sistema de archivos robusto para guardar configuraciones e iconos personalizados sin límites de NVS.
- 🌐 **Panel Web Completo**: Configura botones, iconos, colores de fondo y Red WiFi cómodamente desde tu navegador.
- 🔄 **Actualización Web OTA**: Sube nuevos firmwares (.bin) directamente desde el panel web, sin cables.
- 🔐 **Seguridad Simplificada**: Configurado para emparejamiento fácil sin errores de SMP.
- 💡 **Control de Brillo**: Ajuste táctil en tiempo real.

## 🚀 Inicio Rápido

### 1. Requisitos

- [PlatformIO](https://platformio.org/install/ide?install=vscode) (Recomendado VS Code)
- Dispositivo PandaTouch (BigTreeTech)

### 2. Compilar y Subir

```bash
# Limpiar y compilar
pio run -t clean
pio run

# Subir al dispositivo (ajusta el puerto COM según sea necesario)
pio run -t upload --upload-port COM9
```

### 3. Emparejamiento

1. En Windows, ve a **Configuración > Bluetooth y dispositivos**.
2. Dale a **Agregar dispositivo** -> **Bluetooth**.
3. Selecciona **"PandaTouch Deck"**.
4. Una vez conectado, el PandaTouch mostrará en el puerto serial el mensaje de éxito y estará listo para usar.


### 3. Build & upload

```bash
# Build
pio run

# Upload (replace port with yours)
pio run -t upload -e pandatouch --upload-port /dev/ttyUSB0

# Monitor
pio device monitor -b 115200
```

<a id="using-the-template"></a>

## 🖥️ Using the template

Below are a few small LVGL examples to get you started. The template initializes the display, touch, and LVGL for you — call `pt_setup_display([Render mode])` in `setup()` and be sure to call `pt_loop_display()` regularly from `loop()` so LVGL can run its timers and process touch events.

For more details, see [Render mode quick reference](#render-mode-quick-ref).

Basic setup/loop skeleton:

```cpp
#include "pt/pt_display.h"
// #include "pt_demo.h"

void setup() {
  // Choose a render mode at init (examples in pt_display.h):
  // PT_LVGL_RENDER_FULL_1, PT_LVGL_RENDER_FULL_2,
  // PT_LVGL_RENDER_PARTIAL_1, PT_LVGL_RENDER_PARTIAL_2 (default),
  // PT_LVGL_RENDER_PARTIAL_1_PSRAM, PT_LVGL_RENDER_PARTIAL_2_PSRAM
  pt_setup_display(PT_LVGL_RENDER_FULL_1); // init LCD, touch, LVGL using full-frame in PSRAM
  // run the provided demo
  // pt_demo_create_brightness_demo();
}

void loop() {
  pt_loop_display(); // must be called regularly: runs lv_timer_handler and processes touch
}
```

1. Create a simple label:

```cpp
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "Hello PandaTouch!");
lv_obj_center(label);
```

2. Create a button with an event callback:

```cpp
static void btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    lv_label_set_text(lbl, "Clicked!");
  }
}

lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

lv_obj_t *btn_label = lv_label_create(btn);
lv_label_set_text(btn_label, "Press me");
lv_obj_center(btn_label);
```

> Notes:
>
> - Always call `pt_loop_display()` from `loop()` — the template's LVGL integration depends on it to run timers and to feed touch events.
> - You can set the render mode globally at compile time via a build flag. Example (PlatformIO):
>
> ```ini
> build_flags = -DPT_LVGL_RENDER_METHOD=PT_LVGL_RENDER_FULL_1
> ```

Passing the mode to `pt_setup_display(...)` in code overrides the compile-time default for that call.

<a id="render-mode-quick-ref"></a>

## 🧩 Render mode quick reference

| Method                           |                   Approx memory footprint | Recommended when...                                                          |
| -------------------------------- | ----------------------------------------: | ---------------------------------------------------------------------------- |
| `PT_LVGL_RENDER_FULL_1`          |              ~1x full framebuffer (PSRAM) | You have PSRAM and want full-frame rendering with minimal internal RAM usage |
| `PT_LVGL_RENDER_FULL_2`          |              ~2x full framebuffer (PSRAM) | You have abundant PSRAM and need double-buffering to avoid tearing           |
| `PT_LVGL_RENDER_PARTIAL_1`       | small partial buffer (internal preferred) | Internal RAM available but PSRAM is scarce; memory efficient                 |
| `PT_LVGL_RENDER_PARTIAL_2`       |   2x partial buffers (internal preferred) | Want smoother flushes with limited RAM usage (default)                       |
| `PT_LVGL_RENDER_PARTIAL_1_PSRAM` |             small partial buffer in PSRAM | Internal RAM limited, PSRAM available; slightly slower flushes               |
| `PT_LVGL_RENDER_PARTIAL_2_PSRAM` |               2x partial buffers in PSRAM | Balance between smoothness and PSRAM usage                                   |

> Note on partial buffer height:
>
> - The number of scanlines used for partial buffers is controlled by the macro `PT_LVGL_RENDER_PARTIAL_LINES`. The default value in the code is `80` (see `pt/pt_display.h`). Larger values increase memory use but reduce the number of flushes; smaller values are more memory-efficient but may increase flush frequency.
>
> To override the default at compile time with PlatformIO, add a build flag, for example:
>
> ```ini
> build_flags = -DPT_LVGL_RENDER_PARTIAL_LINES=120
> ```

> Note on LCD bounce buffer size:
>
> - The driver may allocate a small "bounce" area used internally by the RGB panel driver for transient pixel buffering. The number of lines used for this bounce buffer is controlled by `PT_LCD_RENDER_BOUNCE_LINES` (default `10` in `pt/pt_display.h`). The actual pixel allocation equals `PT_LCD_RENDER_BOUNCE_LINES * PT_LCD_H_RES` and is passed to the panel driver as a small scratch buffer. Increase it if you see tearing/artifacts during panel bring-up; decreasing it saves a little RAM.
>
> To override at compile time with PlatformIO:
>
> ```ini
> build_flags = -DPT_LCD_RENDER_BOUNCE_LINES=20
> ```

<a id="arduino-core-selection"></a>

## 🧩 Arduino Core Selection (2.x vs 3.x) — How this template handles it

This template provides two ready-to-use PlatformIO environments in `platformio.ini` so you can choose the Arduino core that fits your dependencies and hardware needs.

Short story:

- `pandatouch` — default environment using the PlatformIO-bundled Arduino 2.x compatible setup (widest library support).
- `pandatouch-arduino-3x` — environment pinned to Arduino core 3.x (3.0.7) and paired SDK libs; useful for ESP32‑S3 + RGB LCD performance and newer USB features.

Why two envs?

- Some third-party Arduino libraries still expect the 2.x core. The 3.x core improves S3 support (USB, timing) but can break a few libraries.

How to use the environments

- The template's default environment is `pandatouch`. Running `pio run` without `-e` will build that environment.

Example commands:

```bash
# Build the default env (pandatouch)
pio run

# Upload using the default env (replace port)
pio run -t upload --upload-port /dev/ttyUSB0

# Build using the 3.x pinned environment
pio run -e pandatouch-arduino-3x

# Upload using the 3.x env (replace port)
pio run -e pandatouch-arduino-3x -t upload --upload-port /dev/ttyUSB0
```

> Notes on what changes between envs
>
> - The environments have different `lib_deps`, and the 3.x env also declares `platform_packages` pointing to the Arduino-ESP32 3.0.7 GitHub tree and prebuilt SDK libs. That forces PlatformIO to use the 3.x core while keeping the rest of your project unchanged.
> - Tip: list all envs in your `platformio.ini` with:
>
> ```bash
> pio run --list
> ```

### Recommended defaults for PandaTouch

```ini
build_flags =
  -I include
  -DLV_CONF_INCLUDE_SIMPLE
  -DBOARD_HAS_PSRAM

board_build.arduino.memory_type = qio_opi
board_build.f_flash = 80000000L
board_build.flash_mode = qio
```

- `BOARD_HAS_PSRAM` → allows LVGL to allocate larger buffers in PSRAM.
- `qio_opi` + `80 MHz` flash → S3 + Octal PSRAM modules.
- `LV_CONF_INCLUDE_SIMPLE` → include `lv_conf.h` in `include/`.

<a id="common-pitfalls"></a>

### Common pitfalls (and fixes)

-- “Why did my libs disappear?”  
 You probably used `lib_deps =` more than once in the same environment. Put all required libraries for an environment in a single `lib_deps` block to avoid accidentally overwriting earlier entries.

- Switched envs and something won’t compile?  
  Build the env that worked previously (for example `pio run -e pandatouch`) to confirm. If the 3.x env fails, either use the 2.x env or pin alternative library versions in `lib_deps` under the 3.x env.

- USB confusion
  - **USB‑C** on the board is for **power + flashing/serial**.
  - **USB‑A** is **OTG** (host/device) and is controlled in software.

<a id="example-platformioini"></a>

### Example: full `platformio.ini` skeleton (multi-env)

Below is the actual multi-environment `platformio.ini` used by this template. It exposes two working envs so you can switch between the PlatformIO-bundled 2.x setup and a pinned 3.x setup without manual edits.

```ini
; ================================================================
; PandaTouch – PlatformIO Configuration
; ================================================================

[platformio]
default_envs = pandatouch

[env]
platform = espressif32@6.12.0
framework = arduino
board = esp32-s3-devkitc-1
monitor_speed = 115200
lib_deps =
  lvgl/lvgl@9.3.0
  tamctec/TAMC_GT911@1.0.2
build_flags =
  -I include
  -DLV_CONF_INCLUDE_SIMPLE
  -DBOARD_HAS_PSRAM

board_build.arduino.memory_type = qio_opi
board_build.f_flash = 80000000L
board_build.flash_mode = qio


[env:pandatouch]
lib_deps =
  lvgl/lvgl@9.3.0
  tamctec/TAMC_GT911@1.0.2
  moononournation/GFX Library for Arduino@1.5.0

[env:pandatouch-arduino-3x]
lib_deps =
  lvgl/lvgl@9.3.0
  tamctec/TAMC_GT911@1.0.2
  moononournation/GFX Library for Arduino@1.6.1
platform_packages =
  framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32.git#3.0.7
  platformio/framework-arduinoespressif32-libs @ https://dl.espressif.com/AE/esp-arduino-libs/esp32-3.0.7.zip
extra_scripts = pre:build_files_exclude.py
custom_build_files_exclude = */Arduino_ESP32LCD8.cpp */Arduino_ESP32QSPI.cpp

```

<a id="hardware-specs"></a>

## 📋 Hardware specs

### MCU

- **ESP32-S3** (dual-core Xtensa LX7 @ 240 MHz)
- Wi-Fi 2.4 GHz, Bluetooth 5 LE
- 8 MB **Octal PSRAM** onboard
- Native USB-C for flashing & power
- USB-A port for OTG device/host

### LCD (RGB Parallel, 800×480)

| Signal    | GPIO           | Notes                |
| --------- | -------------- | -------------------- |
| PCLK      | 5              | Pixel clock          |
| DE        | 38             | Data enable          |
| HSYNC     | –              | Not used (DE mode)   |
| VSYNC     | –              | Not used (DE mode)   |
| R3–R7     | 6–10           | Red data             |
| G2–G7     | 11–16          | Green data           |
| B3–B7     | 17,18,48,47,39 | Blue data            |
| Backlight | 21             | PWM backlight (LEDC) |
| Reset     | 46             | LCD reset            |

### Touch (GT911, I²C0)

| Signal | GPIO |
| ------ | ---- |
| SCL    | 1    |
| SDA    | 2    |
| IRQ    | 40   |
| RST    | 41   |

### I²C1 (example: AHT20 sensor)

- SCL: GPIO3
- SDA: GPIO4

### USB

- USB-C: Power + flashing
- USB-A: OTG (host/device)
- D−: GPIO19
- D+: GPIO20

<a id="troubleshooting"></a>

## ⚠️ Troubleshooting

- **Build errors (missing libs):** run `pio update` and recheck `lib_deps`.
- **Out of memory:** confirm PSRAM is enabled (`BOARD_HAS_PSRAM`) and use partial LVGL buffers in `lv_conf.h`.
- **Upload issues on macOS:** check port (`/dev/cu.usbserial-XXXX`) and close other serial monitors.

<a id="resources"></a>

## 📚 Resources

- [LVGL Docs](https://docs.lvgl.io/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [ESP32-S3 (espressif32)](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s3-devkitc-1.html)
- [LCD Datasheet](docs/QX05ZJGI70N-05545Y.pdf)
- [Pinout reference](docs/PINOUT.md)

<a id="faq"></a>

## ❓ FAQ

**Q: Which core should I start with?**  
A: Start with **2.x** if you depend on many third‑party Arduino libs. Switch to **3.x** for best ESP32‑S3 + LCD performance once your dependencies are compatible.

**Q: How do I revert to 2.x?**  
A: The template exposes a ready-to-use 2.x environment named `pandatouch`. Build with that env to use the PlatformIO-bundled Arduino 2.x setup:

```bash
# Build using the 2.x env
pio run -e pandatouch

# Upload using the 2.x env
pio run -e pandatouch -t upload --upload-port /dev/ttyUSB0
```

If you prefer to keep a single `platformio.ini` env and manually edit it, you can still remove/comment the `platform_packages` block and any 3.x-specific `lib_deps` and then run a clean build (`pio run -t clean`) before rebuilding.

Alternatively, copy the contents of `platformio.example.ini` to your `platformio.ini` to restore a standalone 2.x configuration.

**Q: Can I mix `lib_deps` blocks?**  
A: It's best to list all libraries for an environment in one `lib_deps` block. If you include multiple `lib_deps =` blocks in the same `[env]`, a later one will overwrite earlier entries.
