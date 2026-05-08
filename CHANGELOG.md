# Changelog - PandaTouch StreamDeck (Fork)

All improvements and changes made in this enhanced version.

## [v1.6.1] - 2026-05-08
### New Features
- **Codebase Modularization**: Complete refactor of monolithic `streamdeck.cpp` (2340 lines) into 7 focused modules with headers.
  - `constants.h` – shared enums and limits (`MAX_BUTTONS 20`, `ButtonType`, `TargetOS`, `KbLang`)
  - `storage.h/.cpp` – NVS + LittleFS config load/save/migrate
  - `ble_actions.h/.cpp` – BLE keyboard, shortcuts, button actions
  - `l10n.h/.cpp` – English/Spanish localization + FontAwesome symbol tables
  - `ui_helpers.h/.cpp` – reusable selection screen component
  - `ui_main.h/.cpp` – main screen + OTA update screen
  - `ui_settings.h/.cpp` – settings/edit/wifi screens with callbacks
  - `webserver.h/.cpp` + `webserver_html.h` – REST API + dashboard HTML
  - `streamdeck.h/.cpp` – facade (~50 lines) delegating to modules
- **Web Dashboard Icons**: Icon selector dropdowns now display all 20 FontAwesome symbols with preview characters.

### Bug Fixes
- **Restore Backup – Data Corruption** (critical): `load_settings()` was called inside the restore handler, overwriting all values parsed from the backup JSON with stale NVS data. Now reads only the active bin file directly.
- **Restore Backup – HTTP 500**: ESPAsyncWebServer sends 500 when `_onRequest` is NULL. Restructured to request-handler + body-handler with body accumulated in `_tempObject`.
- **Restore Backup – `String*` Cast UB** (critical): `_tempObject` was malloc'd as raw `char*` but cast to `String*`, causing undefined behavior. Fixed to proper `char*` with null-termination.
- **Restore Backup – Memory Leak**: `_tempObject` was never freed after restore. Now properly freed in all exit paths.
- **Boot – Config Not Loaded**: `load_settings()` was never called at startup. Added to `StreamDeckApp::setup()`.
- **OS Migration – Data Loss**: `init_os_v4` migration overwrote existing `.bin` files even when they already existed. Now preserves existing files.
- **Color Slider LVGL 9.x**: `color_slider_cb` now uses `lv_color_make(r,g,b)` instead of struct member assignment, fixing RGB565 compatibility.
- **Toolchain Windows**: Renamed cross-compiler executables (added prefixed copies) to work around GCC 8.4.0 limitation on Windows.
- **Icon Mapping in Restore**: Added case-insensitive icon-name to FontAwesome-code conversion in restore handler, matching `/api/save` behavior. Unknown names preserved raw for backwards compatibility.

### Improvements
- **Backup JSON**: Added `wifi_pass` to backup so WiFi credentials survive restore.
- **Input Validation**: Rows/cols clamped to 1–5 in all layers (web API, NVS load, UI grid builder).
- **Restore Size Limit**: Increased `MAX_RESTORE_SIZE` from 100KB to 512KB with matching frontend validation.
- **Debug Logging**: Added detailed serial logs at every step of the restore process (chunks, JSON parse, NVS write, bin files, assets, icon mapping).
- **LVGL Config Optimization**: Disabled unused widgets, reduced log level to WARN.

## [v1.6.0] - 2026-02-01
### Bug Fixes
- **OTA Update Stability**: Fixed critical OTA (Over-The-Air) update issue where firmware uploads would fail with "premature end" error on multipart form-data requests.
  - Root cause: Server was validating against `Content-Length` which includes multipart boundaries (~202 bytes overhead), not actual file size.
  - Solution: Changed from `Update.begin(exact_size)` to `Update.begin(UPDATE_SIZE_UNKNOWN)` to let ESP32's Update API handle multipart parsing automatically.
  - Result: Firmware updates now complete successfully with automatic integrity validation.
- **Web Interface Button Configuration**: Fixed critical issue where button configuration settings were not being saved or applied correctly through the web dashboard.
  - Root cause: JavaScript form submission and state management errors preventing proper data serialization and server communication.
  - Solution: Corrected form handling, improved validation, and enhanced server-side configuration persistence.
  - Result: Button configuration now saves reliably and changes apply immediately on the device.

### Improvements
- **Web Upload Progress**: Enhanced client-side upload progress handling to prevent premature connection closure.
  - Limited progress bar updates to 250ms intervals to avoid overwhelming the server.
  - Capped progress bar at 99% until server confirms successful completion (prevents client closing connection too early).
  - Extended XHR timeout from 3 to 5 minutes to allow sufficient time for flash write operations.
- **Flash Write Synchronization**: Improved server-side synchronization during firmware writes.
  - Increased yield/delay iterations (from 100 to 200) to give SPI/flash controller time to complete writes.
  - Added 2-second final settling time before calling `Update.end()` to ensure all buffered data reaches flash memory.
  - Replaced MD5 verification with full Update.end(true) validation for better reliability.
- **Multipart Form-Data Handling**: Removed strict size pre-validation that incorrectly counted multipart overhead.
  - Now displays informative logs about multipart boundaries without treating them as errors.
  - Delegates firmware validation to ESP32's Update API which is more robust.
- **Diagnostic Logging**: Enhanced serial output with detailed multipart/form-data information for troubleshooting.
  - Logs Content-Length (including overhead), actual bytes written, and firmware size in MB.
  - Clear explanations when multipart boundaries are encountered.

### Technical Details
- Changed HTTP request handling from strict Content-Length validation to flexible multipart parsing.
- Timeout value: `xhr.timeout = 300000` (5 minutes, up from 180000/3 minutes).
- Yield pattern: 200 iterations × 100µs = better distributed flash write sync.
- Final flush: 200 iterations × 2000µs = 400ms + 2 second delay = 2.4 seconds total.
- Firmware size validation: Now happens during `Update.end(true)` instead of at upload start.
- Web interface: Improved JavaScript form state management and data serialization for button configuration.

## [v1.5.4] - 2026-01-31
### Bug Fixes
- **Image Restore**: Fixed corruption issue when restoring custom images from backup thanks to a more robust Base64 decoder.

## [v1.5.0] - 2026-01-31
### Bug Fixes
- **Definitive Hybrid Fix**: Successfully matched the original BTT firmware's 80MHz QIO/OPI performance.
- **Boot Stability**: Using DIO mode for the bootloader header to ensure 100% reliable starts on all hardware units (fixes `ets_loader.c 78`).
- **Screen Performance**: Fixed interference and artifacts by correctly mapping the Octal PSRAM at 80MHz with the `qio_opi` memory type.
- **Size Unification**: Locked 16MB flash size across all build and release stages.

## [v1.4.0] - 2026-01-30
### New Features
- **Multi-language Support (EN/ES)**: Full localization system for both the web dashboard and the on-device display. Toggle between English and Spanish seamlessly.
- **Unified Backup & Restore**: The backup system now includes all custom images from LittleFS as Base64 encoded strings within a single JSON file. One-click solution to restore your entire setup, including graphics.
- **Improved Web Dashboard**: Added descriptive labels for "Basic Combo" and localized all interface elements, including icon names and tooltips.

### Improvements & Fixes
- **UI Clarification**: Renamed "Combo Básico" and added helpful hints in the web UI for better user guidance.
- **Standardized UI**: Improved consistency of button sizes and labels across different languages on the device screen.
- **Localized OTA Updates**: Firmware update progress messages are now shown in the user's selected language.

## [v1.3.3] - 2026-01-29
### Critical Fixes
- **Fix WiFi Crash**: Fixed crash during WiFi initialization (TCP/IP stack) that caused reboots on fresh installations.
- **Fix Watchdog Bootloop**: Added a delay in the main loop to prevent watchdog timer resets.

## [v1.3.2] - 2026-01-29
### Improvements and Fixes
- **Factory Binary**: Automatic generation of `factory.bin` in releases for initial installations without bootloops.
- **Partition Table**: New `partitions_custom.csv` optimized for OTA with 3MB per app slot.
- **Documentation**: Updated README with clear instructions on when to use `factory.bin` vs `firmware.bin`.
- **CI/CD**: GitHub Actions now automatically builds and publishes both binaries.

## [v1.3.1] - 2026-01-29
### New Features
- **Enhanced OTA Update**: New persistent update interface on the device that eliminates flickering, shows a real progress bar, and prevents text overlapping.
- **Advanced Shortcuts (Combos)**: Full support for `CTRL+SHIFT+ALT+Key` combinations.
- **Visual Shortcut Builder**: New panel in the web dashboard to create combos by checking boxes, without manual text entry.
- **Media Controls**: Added `next`, `prev`, and `stop` commands.
- **Visual Icons**: Icon selectors on web and device now show the graphic symbol next to the name.

### Improvements and Fixes
- **Fix Ghost Shift**: Fixed an issue where the HID library would automatically send `Shift` when uppercase characters were detected.
- **Security**: System files `win_btns.bin` and `mac_btns.bin` are now hidden and protected against accidental deletion.
- **PlayPause Icon**: New combined ▶⏸ icon.
- **CI/CD**: Automatic GitHub release notes generation from this Changelog file.

## [v1.2.0] - 2026-01-27
### Added
- **Web OTA Update**: New interface in the control panel to upload `.bin` files and update firmware wirelessly.
- **macOS Support**: Automatic mapping of the Cmd key and Spotlight launcher (`Cmd+Space`).
- **Turbo Typing**: Reduced typing delay to 5ms for an instant "paste" effect.
- **I2C Robustness**: Validated touch coordinates to avoid ghost clicks and reduced I2C speed to 100kHz.
- **MIT License**: Added official license to the repository.

### Changed
- **Storage System**: Migration from NVS to binary files in LittleFS (`/win_btns.bin`, `/mac_btns.bin`) for unlimited profiles.
- **Configuration Interface**: Complete redesign of the button editing screen to prevent overlapping.
- **Interface Colors**: All screens now respect the global background color defined by the user.

### Fixed
- **NVS Out of Space Error**: Resolved via migration to LittleFS.
- **Ghost Clicks**: Solved with coordinate filters in the touch driver.
- **Black Screen**: Added default colors (dark grey) to avoid confusion on first boot.

---
*This fork focuses on stability and cross-platform ease of use.*
