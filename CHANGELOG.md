# Changelog - PandaTouch StreamDeck (Fork)

Todas las mejoras y cambios realizados en esta versión mejorada.

## [1.3.0] - 2026-01-29
### Nuevas Funcionalidades
- **Actualización OTA Mejorada**: Nueva interfaz de actualización persistente en el dispositivo que elimina parpadeos, muestra una barra de progreso real y evita solapamiento de textos.
- **Atajos Avanzados (Combos)**: Soporte completo para combinaciones `CTRL+SHIFT+ALT+Tecla`.
- **Constructor Visual de Atajos**: Panel en el dashboard web para crear combos marcando casillas, sin escribir texto manualmente.
- **Controles Multimedia**: Añadidos comandos `next`, `prev` y `stop`.
- **Iconos Visuales**: Los selectores de iconos en web y dispositivo ahora muestran el símbolo gráfico junto al nombre.

### Mejoras y Correcciones
- **Fix Shift Fantasma**: Corregido error donde la librería HID enviaba `Shift` automáticamente al detectar mayúsculas.
- **Seguridad**: Los archivos de sistema `win_btns.bin` y `mac_btns.bin` ahora están ocultos y protegidos contra borrado accidental.
- **Icono PlayPause**: Nuevo icono combinado ▶⏸.
- **CI/CD**: Generación automática de notas de release en GitHub a partir de este archivo Changelog.

## [1.2.0] - 2026-01-27
### Añadido
- **Actualización Web OTA**: Nueva interfaz en el panel de control para subir archivos `.bin` y actualizar el firmware sin cables.
- **Soporte macOS**: Mapeo automático de la tecla Cmd y lanzador Spotlight (`Cmd+Space`).
- **Escritura Turbo**: Reducción del retardo de escritura a 5ms para un efecto de "pegado" instantáneo.
- **Robustez I2C**: Validación de coordenadas táctiles para evitar clics fantasma y reducción de velocidad I2C a 100kHz.
- **Licencia MIT**: Añadida licencia oficial al repositorio.

### Cambiado
- **Sistema de Almacenamiento**: Migración de NVS a archivos binarios en LittleFS (`/win_btns.bin`, `/mac_btns.bin`) para perfiles ilimitados.
- **Interfaz de Configuración**: Rediseño completo de la pantalla de edición de botones para evitar solapamientos.
- **Colores de Interfaz**: Ahora todas las pantallas respetan el color de fondo global definido por el usuario.

### Corregido
- **Error NVS Out of Space**: Solucionado mediante la migración a LittleFS.
- **Click Fantasmas**: Solucionado con filtros de coordenadas en el driver táctil.
- **Pantalla Negra**: Añadidos colores por defecto (gris oscuro) para evitar confusión en el primer arranque.

---
*Este fork se centra en la estabilidad y la facilidad de uso multiplataforma.*
