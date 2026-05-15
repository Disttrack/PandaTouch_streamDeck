# Operational Guide - PandaTouch StreamDeck ESP32

## Quick Commands

| Task | Command | Notes |
|------|---------|-------|
| Build | `pio run -t build` | Standard build |
| Upload (first boot) | `$ pio run -t upload` | **USB-C only** for first install |
| Test OTA API | `curl -X POST http://192.168.X.X/api/update --data-binary @"firmware.bin"` | Use after pairing |
| Test Restore API | `curl -X POST http://192.168.X.X/api/restore` | Load from /backup/restore.json |
| Upload assets | `curl -X POST http://192.168.X.X/api/files @iconset.json` | JSON base64 array |
| Delete file | `curl -X POST "http://192.168.X.X/api/delete?filename=/assets/foo.png"` | System files forbidden |

## Commands You Must Run (Order Matters)

### OTA Update Flow (Critical!)
```bash
# 1️⃣ First install: USB + factory.bin with partition fix
$ pio run -t upload  # Device restarts after flash

# 2️⃣ After pairing: WiFi + API + firmware.bin only  
curl -X POST http://<IP>/api/upload --data-binary @"firmware.bin" && curl http://<IP>

# ⚠️ NEVER OTA directly to v1.5.x or earlier (premature end bug)
```

### Restore Backup Flow
```bash
# Upload /backup/restore.json via WiFi → Calls api/restore with content in body
curl -X POST http://<IP>/api/restore --data-binary @"/backup/restore.json"

# Expected behavior:
# 1. Base64 assets decoded to /assets/*, binary files written (win_btns.bin/mic_btns.bin)
# 2. g_configs[] loaded from active OS bin file → save_settings() → NVS updated
# 3. UI refresh scheduled for next draw cycle
```

## Architecture Facts

### App Boundaries
| File | Purpose | Key Functions |
|------|---------|---------------|
| `src/webserver.cpp` | Web server (OTA/API) | Handles `/api/upload`, `/api/restore`, `/api/update` |
| `src/streamdeck.cpp` | Screen rendering | Handles LVGL display + button states |
| `src/storage.cpp` | Storage/NVS/LittleFS | Handles settings persistence, batch guards |

### State Management
- **NVS**: Device ID, WiFi credentials, brightness, layout, lang, bg color
- **LittleFS**: Button configs (`win_btns.bin`, `mac_btns.bin`), assets, backups
- **RAM**: `g_configs[]` array holds current button state (max 30 buttons per file)

### Critical API Patterns

#### Batch Mode Guard Pattern (Must Match!)
Every handler that changes state MUST follow:
```cpp
if (!set_pending_redundant_write(true)) {
    save_settings(!isOSSwitch);
    load_settings();
    set_pending_redundant_write(false);
} else {
    save_settings(!isOSSwitch);  // Skip reload if batch active
}
g_pending_ui_update = true;
```

**❌ WRONG** - Missing `set_pending_redundant_write()` guard:
```cpp
asyncBodyHandler, [], [](AsyncWebServerRequest* r, size_t i, uint8_t* d, size_t l, size_t t) {
    if (!i && t > 0) {  // <-- NO BATCH GUARD → causes UI glitch on restore
        g_pending_ui_update = true;
    }
    ...
}
```

### Restore API Special Case (v1.5.4+)
The `/api/restore` handler has custom multipart logic:
- **Accumulates body** into `request->_tempObject` (no Content-Length assumed)
- Max size defaults to 8MB (`MAX_RESTORE_SIZE`)
- Base64 decoder parses chunked payloads
- Writes assets, then loads active OS button bin, saves NVS

### File System Paths
| Path | Contains | Notes |
|------|----------|-------|
| `/win_btns.bin` | Windows HID profile | Default for `g_target_os == 1` |
| `/mac_btns.bin` | macOS HID profile | Default for `g_target_os == 2` |
| `/assets/*.png` | Button icons | Base64 uploaded via `/api/upload`, `/api/files` |
| `/backup/restore.json` | Full device backup | Used by restore API |

## Common Failure Modes

### Boot Loop After First Install
- **Cause**: Used WiFi upload without correct partition table
- **Fix**: Flash `factory.bin` via USB with DIO mode, offset 0x0 only on first boot
- **Warning**: QIO mode only valid after factory.bin installed initial partitions

### OTA Fails on Old Firmware
- **Cause**: Premature end bug in v1.5.x and earlier
- **Fix**: Upgrade to v1.6.0+ via USB first (fixed version handles `Update.end(true)` properly)

## Style Conventions

- **Header constants**: `#define MAX_BUTTONS      30`
- **Color format**: 24-bit hex integers (`0xRRGGBB`)
- **LVGL config**: In `include/lv_conf.h`, uses v9.x API only (v8 breaks displays)
- **Batch guard**: Must wrap all state-modifying web handlers
