#include "streamdeck.h"
#include <BleKeyboard.h>
#include <BLEDevice.h>
#include <BLESecurity.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>
#include <Update.h>

extern void set_brightness(uint8_t val);

// ==========================================
// CONFIGURATION & STRUCTURES
// ==========================================
struct ButtonConfig {
    char label[16];
    char value[128];
    uint8_t type; // 0: Command (Win+R), 1: Media Key, 2: Key Combo (Ctrl+X)
    uint32_t color;
    char icon[8]; // Stores LVGL Symbol string (e.g. LV_SYMBOL_PLAY)
    char imgPath[32]; // Path to custom image in LittleFS (e.g. "/chrome.png")
};

static ButtonConfig g_configs[20];
static uint32_t g_bg_color = 0x000000;
static uint8_t g_rows = 3;
static uint8_t g_cols = 3;
static uint8_t g_target_os = 0; // 0: Windows, 1: macOS
static char g_wifi_ssid[32] = "";
static char g_wifi_pass[64] = "";
static String g_wifi_status = "Disconnected";
static String g_ip_addr = "0.0.0.0";

Preferences preferences;
BleKeyboard bleKeyboard("PandaTouch Deck", "BigTreeTech", 100);
AsyncWebServer server(80);

// ==========================================
// LOCAL GLOBALS
// ==========================================
static lv_obj_t* g_main_screen = nullptr;
static lv_obj_t* g_settings_screen = nullptr;
static lv_obj_t* g_edit_screen = nullptr;
static lv_obj_t* g_wifi_screen = nullptr;
static lv_obj_t* g_color_picker = nullptr;
static lv_obj_t* g_dd_icon = nullptr; // Icon selector
static uint8_t g_editing_idx = 0;
static bool g_editing_bg = false;
static lv_obj_t *g_slider_r, *g_slider_g, *g_slider_b;
static lv_obj_t *g_preview;
static volatile bool g_pending_ui_update = false;

// ==========================================
// SYMBOL MAPPING
// ==========================================
static const char* g_sym_names[] = {"None", "OK", "Close", "Copy", "Paste", "Cut", "Play", "Pause", "Mute", "Settings", "Home", "Save", "Edit", "File", "Dir", "Plus"};
static const char* g_sym_codes[] = {
    "", 
    "\xEF\x80\x8C", // OK
    "\xEF\x80\x8D", // CLOSE
    "\xEF\x83\x85", // COPY
    "\xEF\x83\xAA", // PASTE
    "\xEF\x83\x84", // CUT
    "\xEF\x81\x8B", // PLAY
    "\xEF\x81\x8C", // PAUSE
    "\xEF\x80\xA6", // MUTE
    "\xEF\x80\x93", // SETTINGS
    "\xEF\x80\x95", // HOME
    "\xEF\x83\x87", // SAVE
    "\xEF\x8C\x84", // EDIT
    "\xEF\x85\x9B", // FILE
    "\xEF\x81\xBB", // DIR
    "\xEF\x81\xA7"  // PLUS
};

static const char* get_symbol_by_index(int idx) {
    if(idx < 0 || idx >= 16) return "";
    return g_sym_codes[idx];
}

static int get_index_by_symbol(const char* sym) {
    if(!sym || sym[0] == '\0') return 0;
    for(int i=1; i<16; i++) {
        if(strcmp(sym, g_sym_codes[i]) == 0) return i;
    }
    return 0;
}

// ==========================================
// SPANISH KEYBOARD MAPPING
// ==========================================
static void bleWriteSpanish(char c) {
    if (!bleKeyboard.isConnected()) return;

    switch (c) {
        case ':':
            bleKeyboard.press(KEY_LEFT_SHIFT);
            bleKeyboard.write('.');
            bleKeyboard.releaseAll();
            break;
        case '/':
            bleKeyboard.press(KEY_LEFT_SHIFT);
            bleKeyboard.write('7');
            bleKeyboard.releaseAll();
            break;
        case '\\':
            bleKeyboard.press(KEY_LEFT_CTRL);
            bleKeyboard.press(KEY_LEFT_ALT);
            bleKeyboard.write('`'); // Matches position of 'º' on many US-interpreted Spanish layouts
            bleKeyboard.releaseAll();
            break;
        case '+':
            bleKeyboard.write(']'); // US ']' is where Spanish '+' is
            break;
        case '*':
            bleKeyboard.press(KEY_LEFT_SHIFT);
            bleKeyboard.write(']');
            bleKeyboard.releaseAll();
            break;
        case '-':
            bleKeyboard.write('/'); // US '/' is where Spanish '-' is
            break;
        case '_':
            bleKeyboard.press(KEY_LEFT_SHIFT);
            bleKeyboard.write('/');
            bleKeyboard.releaseAll();
            break;
        default:
            bleKeyboard.write(c);
            break;
    }
}

// ==========================================
// FORWARD DECLARATIONS
// ==========================================
static void create_main_ui();
static void create_settings_ui();
static void create_edit_ui(uint8_t idx);
static void create_wifi_ui();
static void load_settings();
static void save_settings(bool saveButtons = true);
static void check_bluetooth_internal();
static void check_wifi_internal();
static void init_webserver(); // Start Asset & Config Server
static void btn_event_cb(lv_event_t *e);
static void slider_event_cb(lv_event_t *e);
static void settings_btn_cb(lv_event_t *e);
static void settings_wifi_btn_cb(lv_event_t* e);
static void settings_bg_btn_cb(lv_event_t* e);
static void settings_grid_btn_cb(lv_event_t* e);
static void settings_os_btn_cb(lv_event_t* e);
static void back_to_main_cb(lv_event_t *e);
static void save_edit_cb(lv_event_t *e);
static void save_wifi_cb(lv_event_t *e);
static void edit_btn_select_cb(lv_event_t *e);
static void color_slider_cb(lv_event_t *e);
static void kb_focus_cb(lv_event_t *e);
static const char* get_symbol_by_index(int idx);
static int get_index_by_symbol(const char* sym);

// ==========================================
// STORAGE LOGIC
// ==========================================
static void load_settings() {
    preferences.begin("deck", false);
    g_rows = preferences.getUChar("rows", 3);
    g_cols = preferences.getUChar("cols", 3);
    g_target_os = preferences.getUChar("os", 0);
    g_bg_color = preferences.getUInt("bg", 0x121212);
    
    // Safety check: if bg_color is pure black, default to dark grey to avoid "black screen" confusion
    if (g_bg_color == 0x000000) g_bg_color = 0x121212;

    Serial.printf("STORAGE: NVS Global: Rows=%d, Cols=%d, OS=%d, BG=0x%06X\n", g_rows, g_cols, g_target_os, g_bg_color);

    const char* win_file = "/win_btns.bin";
    const char* mac_file = "/mac_btns.bin";

    // Migration/Init for Profiles (v4: LittleFS)
    if (!preferences.getBool("init_os_v4", false)) {
        Serial.println("Initial Profile Setup (v4 LittleFS): Migrating...");
        
        auto set_defaults = []() {
            for(int i=0; i<20; i++) {
                memset(&g_configs[i], 0, sizeof(ButtonConfig));
                g_configs[i].color = 0x333333;
                strncpy(g_configs[i].label, "Button", 15);
            }
        };

        // Migrate Windows Settings to File
        set_defaults();
        if (preferences.getBytes("w_pA", &g_configs[0], 10 * sizeof(ButtonConfig)) > 0) {
            preferences.getBytes("w_pB", &g_configs[10], 10 * sizeof(ButtonConfig));
        } else {
            for(int i=0; i<20; i++) {
                char k1[8], k2[8];
                sprintf(k1, "b%d", i); sprintf(k2, "wb%d", i);
                if (preferences.getBytes(k2, &g_configs[i], sizeof(ButtonConfig)) == 0)
                    preferences.getBytes(k1, &g_configs[i], sizeof(ButtonConfig));
            }
        }
        File f = LittleFS.open(win_file, "w");
        if (f) { f.write((uint8_t*)g_configs, sizeof(g_configs)); f.close(); }

        // Migrate Mac Settings to File
        set_defaults();
        if (preferences.getBytes("m_pA", &g_configs[0], 10 * sizeof(ButtonConfig)) > 0) {
            preferences.getBytes("m_pB", &g_configs[10], 10 * sizeof(ButtonConfig));
        } else {
            for(int i=0; i<20; i++) {
                char k3[8]; sprintf(k3, "mb%d", i);
                preferences.getBytes(k3, &g_configs[i], sizeof(ButtonConfig));
            }
        }
        f = LittleFS.open(mac_file, "w");
        if (f) { f.write((uint8_t*)g_configs, sizeof(g_configs)); f.close(); }

        preferences.putBool("init_os_v4", true);
        Serial.println("STORAGE: Migration to LittleFS files complete.");
    }

    // Load Buttons for current OS
    const char* active_file = (g_target_os == 0 ? win_file : mac_file);
    Serial.printf("STORAGE: Loading file: %s... ", active_file);
    File f = LittleFS.open(active_file, "r");
    if (f) {
        size_t read = f.read((uint8_t*)g_configs, sizeof(g_configs));
        f.close();
        if (read == sizeof(g_configs)) {
            Serial.println("OK");
        } else {
            Serial.println("FAIL (size mismatch)");
            goto load_defaults;
        }
    } else {
        Serial.println("NOT FOUND");
        load_defaults:
        for(int i=0; i<20; i++) {
            memset(&g_configs[i], 0, sizeof(ButtonConfig));
            g_configs[i].color = 0x333333;
            strncpy(g_configs[i].label, "Button", 15);
        }
    }

    preferences.getString("wssid", g_wifi_ssid, 31);
    preferences.getString("wpass", g_wifi_pass, 63);
    preferences.end();
    
    if (strlen(g_wifi_ssid) > 0) {
        WiFi.begin(g_wifi_ssid, g_wifi_pass);
    }
}

static void save_settings(bool saveButtons) {
    Serial.printf("Saving settings (buttons=%s)...\n", saveButtons ? "YES" : "NO");
    preferences.begin("deck", false);
    preferences.putUInt("bg", g_bg_color);
    preferences.putUChar("rows", g_rows);
    preferences.putUChar("cols", g_cols);
    preferences.putUChar("os", g_target_os);
    preferences.putString("wssid", g_wifi_ssid);
    preferences.putString("wpass", g_wifi_pass);
    preferences.end();

    if (saveButtons) {
        const char* active_file = (g_target_os == 0 ? "/win_btns.bin" : "/mac_btns.bin");
        File f = LittleFS.open(active_file, "w");
        if (f) {
            f.write((uint8_t*)g_configs, sizeof(g_configs));
            f.close();
            Serial.printf("STORAGE: Buttons saved to %s\n", active_file);
        } else {
            Serial.printf("STORAGE ERROR: Failed to open %s for writing\n", active_file);
        }
    }
}

// ==========================================
// PUBLIC API IMPLEMENTATION
// ==========================================
void StreamDeckApp::setup() {
    // 0. LittleFS
    if(!LittleFS.begin(true)){
        Serial.println("LittleFS Mount Failed");
    } else {
        Serial.println("LittleFS Mounted Successfully. Files:");
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        while(file){
            Serial.printf(" - %s (%d bytes)\n", file.name(), file.size());
            file = root.openNextFile();
        }
    }

    // 1. Storage & Config
    load_settings();

    // 2. Init UI
    g_main_screen = lv_scr_act();
    create_main_ui();

    Serial.println("StreamDeckApp::setup() - Starting BLE initialization");
    
    // 2. Configure BLE security - disable to avoid Windows SMP issues
    delay(500); // Give system time to stabilize
    Serial.println("Setting BLE security to NONE...");
    
    // Set security to open (no pairing required)
    // This fixes the SMP errors with Windows
    BLEDevice::init("PandaTouch Deck");
    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_NO_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    
    // 3. Start BLE Keyboard
    Serial.println("Calling bleKeyboard.begin()...");
    bleKeyboard.begin();
    Serial.println("bleKeyboard.begin() completed");
    Serial.println("BLE Keyboard initialized. Device name: PandaTouch Deck");
    Serial.println("Security: NO PAIRING REQUIRED");
    Serial.println("Waiting for Bluetooth connection...");

    // 4. Init OTA
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("OTA: Start updating " + type);
    });
    ArduinoOTA.onEnd([]() { Serial.println("\nOTA: Update Complete"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
}

void StreamDeckApp::loop() {
    check_bluetooth_internal();
    check_wifi_internal();
    
    if (g_pending_ui_update) {
        g_pending_ui_update = false;
        lv_scr_load(g_main_screen);
        create_main_ui();
    }
    
    ArduinoOTA.handle();
}

void StreamDeckApp::handle_button(uint8_t idx) {
    if (!bleKeyboard.isConnected()) {
        Serial.println("BLE not connected!");
        return;
    }

    if (idx >= 20) return;
    ButtonConfig &cfg = g_configs[idx];
    Serial.printf("Executing Button %d: %s (Type: %d)\n", idx, cfg.label, cfg.type);

    if (cfg.type == 0) { // Command (Win+R / Cmd+Space)
        if (g_target_os == 0) { // Windows
            bleKeyboard.press(KEY_LEFT_GUI);
            bleKeyboard.press('r');
            delay(150);
            bleKeyboard.releaseAll();
            delay(500); // Faster Run dialog wait
        } else { // macOS
            bleKeyboard.press(KEY_LEFT_GUI);
            bleKeyboard.press(' ');
            delay(150);
            bleKeyboard.releaseAll();
            delay(300); // Faster Spotlight wait
        }
        
        // Ultra-fast typing with 5ms delay
        for(int i=0; cfg.value[i]; i++) {
            bleWriteSpanish(cfg.value[i]);
            delay(5); 
        }
        
        delay(200);
        bleKeyboard.write(KEY_RETURN);
    } 
    else if (cfg.type == 1) { // Media
        if (strcmp(cfg.value, "mute") == 0) bleKeyboard.write(KEY_MEDIA_MUTE);
        else if (strcmp(cfg.value, "volup") == 0) bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        else if (strcmp(cfg.value, "voldown") == 0) bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        else if (strcmp(cfg.value, "play") == 0) bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
    }
    else if (cfg.type == 2) { // Key Combo (Ctrl+X or Cmd+X)
        if (g_target_os == 0) bleKeyboard.press(KEY_LEFT_CTRL); // Windows
        else bleKeyboard.press(KEY_LEFT_GUI); // macOS (Cmd)
        
        bleKeyboard.press(cfg.value[0]);
        delay(100);
        bleKeyboard.releaseAll();
    }
}

// ==========================================
// INTERNAL HELPERS
// ==========================================
static void check_wifi_internal() {
    static bool was_connected = false;
    static unsigned long last_wifi_check = 0;
    
    if (millis() - last_wifi_check < 2000) return;
    last_wifi_check = millis();

    bool is_connected = (WiFi.status() == WL_CONNECTED);
    
    if (is_connected != was_connected) {
        if (is_connected) {
            g_wifi_status = "Connected";
            g_ip_addr = WiFi.localIP().toString();
            Serial.print("WiFi Connected! IP: "); Serial.println(g_ip_addr);
            init_webserver();
        } else {
            g_wifi_status = "Disconnected";
            g_ip_addr = "0.0.0.0";
            Serial.println("WiFi Disconnected.");
        }
        was_connected = is_connected;
    }
}

static void init_webserver() {
    static bool started = false;
    if (started) return;
    
    // API: Get current config
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
        auto escape_json = [](String s) -> String {
            s.replace("\\", "\\\\");
            s.replace("\"", "\\\"");
            s.replace("\n", "\\n");
            s.replace("\r", "\\r");
            s.replace("\t", "\\t");
            return s;
        };

        auto find_name = [&](const char* code) -> String {
            if (!code || code[0] == '\0') return "None";
            for(int j=0; j<16; j++) {
                if(strcmp(code, g_sym_codes[j]) == 0) return g_sym_names[j];
            }
            return "None";
        };

        String json = "{\"bg\":\"" + String(g_bg_color, HEX) + "\",\"rows\":" + String(g_rows) + ",\"cols\":" + String(g_cols) + ",\"os\":" + String(g_target_os) + ",\"buttons\":[";
        for(int i=0; i<20; i++) {
            json += "{\"label\":\"" + escape_json(String(g_configs[i].label)) + "\",";
            json += "\"value\":\"" + escape_json(String(g_configs[i].value)) + "\",";
            json += "\"type\":" + String(g_configs[i].type) + ",";
            json += "\"color\":\"" + String(g_configs[i].color, HEX) + "\",";
            json += "\"icon\":\"" + find_name(g_configs[i].icon) + "\",";
            json += "\"img\":\"" + escape_json(String(g_configs[i].imgPath)) + "\"}";
            if(i < 19) json += ",";
        }
        json += "]}";
        request->send(200, "application/json", json);
    });

    // API: Update config (Simple params)
    server.on("/api/save", HTTP_POST, [](AsyncWebServerRequest *request){
        auto parse_color = [](String hex) -> uint32_t {
            if(hex.startsWith("#")) hex = hex.substring(1);
            return strtol(hex.c_str(), NULL, 16);
        };

        if(request->hasParam("bg", true)) g_bg_color = parse_color(request->getParam("bg", true)->value());
        if(request->hasParam("rows", true)) g_rows = request->getParam("rows", true)->value().toInt();
        if(request->hasParam("cols", true)) g_cols = request->getParam("cols", true)->value().toInt();
        if(request->hasParam("os", true)) g_target_os = request->getParam("os", true)->value().toInt();

        for(int i=0; i<20; i++) {
            String p = "b" + String(i);
            if(request->hasParam(p + "l", true)) strncpy(g_configs[i].label, request->getParam(p + "l", true)->value().c_str(), 15);
            if(request->hasParam(p + "v", true)) strncpy(g_configs[i].value, request->getParam(p + "v", true)->value().c_str(), 127);
            if(request->hasParam(p + "t", true)) g_configs[i].type = request->getParam(p + "t", true)->value().toInt();
            if(request->hasParam(p + "c", true)) g_configs[i].color = parse_color(request->getParam(p + "c", true)->value());
            
            if(request->hasParam(p + "icon", true)) {
                String iconName = request->getParam(p + "icon", true)->value();
                for(int j=0; j<16; j++) {
                    if(iconName == g_sym_names[j]) {
                        strncpy(g_configs[i].icon, g_sym_codes[j], 7);
                        break;
                    }
                }
            }

            if(request->hasParam(p + "i", true)) {
                String val = request->getParam(p + "i", true)->value();
                if (val.length() > 0 && val != "Ninguno") {
                    if (!val.startsWith("/")) val = "/" + val;
                    strncpy(g_configs[i].imgPath, val.c_str(), 31);
                } else {
                    g_configs[i].imgPath[0] = '\0';
                }
            }
        }
        bool isOSSwitch = (request->hasParam("os", true) && request->params() <= 2); 
        // Note: request->params() includes duplicates or internal ones, but usually if we only send OS, we don't want to save current buttons to the new OS file.
        // Actually, let's be more explicit: if the request has OS, we save global prefs and then load buttons.
        
        save_settings(!isOSSwitch); 
        load_settings(); 
        g_pending_ui_update = true;
        request->send(200, "text/plain", "OK");
    });

    // List files
    server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "[";
        File root = LittleFS.open("/");
        if (root) {
            File file = root.openNextFile();
            bool first = true;
            while(file){
                if(!first) json += ",";
                json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
                file = root.openNextFile();
                first = false;
            }
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    // Delete file
    server.on("/api/delete", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("filename", true)) {
            String fname = request->getParam("filename", true)->value();
            if(!fname.startsWith("/")) fname = "/" + fname;
            if(LittleFS.remove(fname)) {
                Serial.printf("API: Deleted %s\n", fname.c_str());
                request->send(200, "text/plain", "OK");
            } else {
                request->send(500, "text/plain", "Delete failed");
            }
        } else {
            request->send(400, "text/plain", "Missing filename");
        }
    });

    // Firmware Update Handler
    server.on("/api/update", HTTP_POST, [](AsyncWebServerRequest *request){
        bool shouldRestart = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldRestart ? "Update OK. Restarting..." : "Update Failed");
        response->addHeader("Connection", "close");
        request->send(response);
        if(shouldRestart) {
            delay(1000);
            ESP.restart();
        }
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if(!index){
            Serial.printf("OTA: Web Update Start: %s\n", filename.c_str());
            if(!Update.begin(UPDATE_SIZE_UNKNOWN)){
                Update.printError(Serial);
            }
        }
        if(len){
            if(Update.write(data, len) != len){
                Update.printError(Serial);
            }
        }
        if(final){
            if(Update.end(true)){
                Serial.printf("OTA: Web Update Success: %u bytes\n", index+len);
            } else {
                Update.printError(Serial);
            }
        }
    });

    // Upload handler with static file handle
    static File uploadFile;
    server.on("/api/upload", HTTP_POST, [](AsyncWebServerRequest *request){
        Serial.println("API: Upload complete.");
        request->send(200, "text/plain", "Upload OK");
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if(!index){
            if(!filename.startsWith("/")) filename = "/" + filename;
            Serial.printf("API: Receiving file %s\n", filename.c_str());
            uploadFile = LittleFS.open(filename, "w");
        }
        if(len && uploadFile) uploadFile.write(data, len);
        if(final && uploadFile) {
            uploadFile.close();
            Serial.println("API: File saved to LittleFS.");
        }
    });

    // Web Dashboard (UTF-8 Header)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>PandaTouch Dash</title>";
        html += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
        html += "<style>body{background:#121212;color:white}.card{background:#1e1e1e;border:1px solid #333;color:white;margin-bottom:15px}.btn-grid{display:grid;grid-template-columns:repeat(auto-fill, minmax(200px, 1fr));gap:15px}.btn-del{padding:0 5px;color:#ff4444;cursor:pointer;border:none;background:none}.hidden-card{display:none}</style>";
        html += "</head><body class='container py-4'>";
        html += "<div class='d-flex justify-content-between align-items-center mb-4'><h2>🎨 PandaTouch Dash</h2>";
        html += "<div class='d-flex align-items-center gap-3'><div class='d-flex align-items-center gap-2'><label>SO:</label><select id='osSelect' class='form-select form-select-sm' style='width:105px'><option value='0'>Windows</option><option value='1'>macOS</option></select><input type='hidden' id='osInput' name='os' form='configForm'></div>";
        html += "<div class='d-flex align-items-center gap-2'><label>Cuadrícula:</label><select id='gridSelect' class='form-select form-select-sm' style='width:100px'><option value='2x2'>2x2</option><option value='3x2'>3x2</option><option value='3x3'>3x3</option><option value='4x3'>4x3</option><option value='5x3'>5x3</option></select><input type='hidden' id='rowsInput' name='rows' form='configForm'><input type='hidden' id='colsInput' name='cols' form='configForm'></div>";
        html += "<div class='d-flex align-items-center gap-2'><label>Fondo:</label><input type='color' id='globalBg' name='bg' form='configForm' class='form-control form-control-color' style='height:35px'></div></div></div>";
        
        html += "<div class='row'><div class='col-md-9'>";
        html += "<div class='card p-3 mb-4'><h5>Configuración de Botones</h5><form id='configForm'><div class='btn-grid' id='buttonContainer'>";
        for(int i=0; i<20; i++) {
            html += "<div class='card p-2 text-center btn-card' id='card"+String(i)+"'>";
            html += "<b class='mb-2'>Botón " + String(i+1) + "</b>";
            html += "<input type='text' name='b"+String(i)+"l' class='form-control form-control-sm mb-1' placeholder='Nombre' maxlength='15'>";
            html += "<input type='text' name='b"+String(i)+"v' class='form-control form-control-sm mb-1' placeholder='Comando' maxlength='127'>";
            html += "<select name='b"+String(i)+"t' class='form-select form-select-sm mb-1'><option value='0'>App (Win+R)</option><option value='1'>Multimedia</option><option value='2'>Combo Ctrl</option></select>";
            html += "<div class='d-flex gap-1 align-items-center mb-1'>";
            html += "<input type='color' name='b"+String(i)+"c' class='form-control form-control-color flex-grow-1' style='height:30px' title='Color Fondo'>";
            html += "<select name='b"+String(i)+"icon' class='form-select form-select-sm' title='Icono'></select>";
            html += "</div>";
            html += "<select name='b"+String(i)+"i' class='form-select form-select-sm asset-select' title='Imagen Custom'></select>";
            html += "</div>";
        }
        html += "</div><button type='submit' class='btn btn-primary mt-3 w-100'>Guardar Cambios</button></form></div></div>";
        
        html += "<div class='col-md-3'><div class='card p-3'><h5>Librería</h5>";
        html += "<input type='file' id='fileInput' class='form-control form-control-sm mb-2'><button onclick='upload()' class='btn btn-sm btn-success w-100 mb-3'>Subir</button>";
        html += "<ul id='fileList' class='list-group list-group-flush small'></ul></div>";
        html += "<div class='card p-3 mt-3'><h5>Firmware OTA</h5>";
        html += "<p class='small text-secondary'>Selecciona archivo .bin para actualizar el dispositivo.</p>";
        html += "<input type='file' id='otaInput' class='form-control form-control-sm mb-2' accept='.bin'>";
        html += "<button onclick='updateFirmware()' class='btn btn-sm btn-warning w-100'>Actualizar</button>";
        html += "<div class='progress mt-2' style='height: 10px; display:none;' id='otaProgressContainer'><div id='otaProgressBar' class='progress-bar progress-bar-striped progress-bar-animated bg-warning' style='width: 0%'></div></div>";
        html += "</div></div></div>";
        
        html += "<script>";
        html += "const SYMBOLS = ['None','OK','Close','Copy','Paste','Cut','Play','Pause','Mute','Settings','Home','Save','Edit','File','Dir','Plus'];";
        html += "document.getElementById('osSelect').onchange = async (e) => {";
        html += " document.getElementById('osInput').value = e.target.value;";
        html += " const fd = new FormData(); fd.append('os', e.target.value);";
        html += " await fetch('/api/save', {method:'POST', body:fd});";
        html += " load();"; // Reload the whole config for the new OS
        html += "};";
        html += "document.getElementById('gridSelect').onchange = (e) => {";
        html += " const [c, r] = e.target.value.split('x').map(Number);";
        html += " document.getElementById('rowsInput').value = r;";
        html += " document.getElementById('colsInput').value = c;";
        html += " updateVisibleCards(r, c);";
        html += "};";
        html += "function updateVisibleCards(r, c) {";
        html += " const count = r * c;";
        html += " for(let i=0; i<20; i++) {";
        html += "  const card = document.getElementById('card'+i);";
        html += "  if(card) card.style.display = (i < count) ? 'block' : 'none';";
        html += " }";
        html += "}";
        html += "async function load(){";
        html += " try {";
        html += "  const r = await fetch('/api/config'); const d = await r.json();";
        html += "  const f = await fetch('/api/files'); const files = await f.json();";
        html += "  document.getElementById('globalBg').value = '#' + d.bg.padStart(6,'0');";
        html += "  document.getElementById('gridSelect').value = d.cols + 'x' + d.rows;";
        html += "  document.getElementById('rowsInput').value = d.rows;";
        html += "  document.getElementById('colsInput').value = d.cols;";
        html += "  document.getElementById('osSelect').value = d.os;";
        html += "  document.getElementById('osInput').value = d.os;";
        html += "  updateVisibleCards(d.rows, d.cols);";
        html += "  const selects = document.querySelectorAll('.asset-select');";
        html += "  selects.forEach(s => { s.innerHTML = '<option value=\"\">Sin Imagen</option>'; files.forEach(file => s.innerHTML += `<option value='${file.name}'>${file.name}</option>`); });";
        html += "  d.buttons.forEach((b,i) => { ";
        html += "   const lbl = document.getElementsByName(`b${i}l`)[0]; if(!lbl) return;";
        html += "   lbl.value = b.label;";
        html += "   document.getElementsByName(`b${i}v`)[0].value = b.value;";
        html += "   document.getElementsByName(`b${i}t`)[0].value = b.type;";
        html += "   document.getElementsByName(`b${i}c`)[0].value = '#' + b.color.padStart(6,'0');";
        html += "   const sIcon = document.getElementsByName(`b${i}icon`)[0];";
        html += "   sIcon.innerHTML = SYMBOLS.map(s => `<option value='${s}'>${s}</option>`).join('');";
        html += "   sIcon.value = b.icon || 'None';";
        html += "   document.getElementsByName(`b${i}i`)[0].value = b.img.startsWith('/') ? b.img.substring(1) : b.img;";
        html += "  });";
        html += "  const fl = document.getElementById('fileList'); fl.innerHTML = '';";
        html += "  files.forEach(file => fl.innerHTML += `<li class='list-group-item bg-dark text-white d-flex justify-content-between align-items-center px-2' style='border-color:#333'>${file.name} <button onclick=\"del('${file.name}')\" class='btn-del'>×</button></li>`);";
        html += " } catch(e) { console.error(e); }";
        html += "}";
        html += "document.getElementById('configForm').onsubmit = async (e) => { ";
        html += " e.preventDefault(); const fd = new FormData(e.target); await fetch('/api/save', {method:'POST', body:fd}); alert('Configuración guardada!'); load();";
        html += "};";
        html += "async function upload(){";
        html += " const fi = document.getElementById('fileInput'); if(!fi.files[0]) return; const fd = new FormData(); fd.append('file', fi.files[0]); await fetch('/api/upload', {method:'POST', body:fd}); load();";
        html += "}";
        html += "async function del(name){ if(!confirm('¿Borrar '+name+'?')) return; const fd = new FormData(); fd.append('filename', name); await fetch('/api/delete', {method:'POST', body:fd}); load(); }";
        html += "async function updateFirmware() {";
        html += " const file = document.getElementById('otaInput').files[0]; if(!file) return; if(!confirm('¿Deseas actualizar el firmware? El dispositivo se reiniciará.')) return;";
        html += " const fd = new FormData(); fd.append('update', file);";
        html += " const xhr = new XMLHttpRequest();";
        html += " xhr.open('POST', '/api/update', true);";
        html += " document.getElementById('otaProgressContainer').style.display = 'block';";
        html += " xhr.upload.onprogress = (e) => { if(e.lengthComputable) { const p = (e.loaded / e.total) * 100; document.getElementById('otaProgressBar').style.width = p + '%'; } };";
        html += " xhr.onload = () => { if(xhr.status === 200) { alert('Actualización completada. Reiniciando...'); location.reload(); } else { alert('Error en la actualización.'); document.getElementById('otaProgressContainer').style.display = 'none'; } };";
        html += " xhr.send(fd);";
        html += "}";
        html += "load();</script></body></html>";
        request->send(200, "text/html; charset=utf-8", html);
    });

    server.begin();
    started = true;
    Serial.println("Web Server started.");
}

static void check_bluetooth_internal() {
    static bool was_connected = false;
    static unsigned long last_check = 0;
    bool is_connected = bleKeyboard.isConnected();
    
    // Check connection state changes
    if (is_connected != was_connected) {
        if (is_connected) {
            Serial.println("*** BLE CONNECTED! ***");
        } else {
            Serial.println("*** BLE DISCONNECTED ***");
        }
        was_connected = is_connected;
    }
    
    // Periodic status update when not connected
    if (!is_connected && (millis() - last_check > 10000)) {
        Serial.println("Still advertising... Waiting for connection.");
        last_check = millis();
    }
}

// ==========================================
// UI - MAIN SCREEN
// ==========================================
static void create_main_ui() {
    lv_obj_clean(g_main_screen);
    lv_obj_set_style_bg_color(g_main_screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

    // Dynamic Grid Descriptors
    static int32_t col_dsc[10];
    static int32_t row_dsc[10];
    
    // 800x480 screen. Footer is ~60px.
    // We calculate cell sizes to fill the space minus padding/gaps.
    int32_t availW = 800 - 20 - (g_cols - 1) * 10;
    int32_t availH = 480 - 20 - 60 - g_rows * 10;
    
    int32_t cellW = availW / g_cols;
    int32_t cellH = availH / g_rows;

    for (int i = 0; i < g_cols; i++) col_dsc[i] = cellW;
    col_dsc[g_cols] = LV_GRID_TEMPLATE_LAST;
    
    for (int i = 0; i < g_rows; i++) row_dsc[i] = cellH;
    row_dsc[g_rows] = 60; // Footer row
    row_dsc[g_rows + 1] = LV_GRID_TEMPLATE_LAST;

    lv_obj_t *grid = lv_obj_create(g_main_screen);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));
    lv_obj_center(grid);
    lv_obj_set_style_bg_color(grid, lv_color_hex(g_bg_color), LV_PART_MAIN);
    lv_obj_set_style_border_width(grid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(grid, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(grid, 10, LV_PART_MAIN);
    
    int btn_count = g_rows * g_cols;
    for (int i = 0; i < btn_count; i++) {
        lv_obj_t *btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % g_cols, 1, LV_GRID_ALIGN_STRETCH, i / g_cols, 1);
        lv_obj_set_style_bg_color(btn, lv_color_hex(g_configs[i].color), LV_PART_MAIN);
        
        // Layout: Create vertical flex for icon + label
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(btn, 5, 0); // Add gap between icon/image and label

        // Icon/Image Logic
        bool icon_or_img_present = false;
        if (g_configs[i].imgPath[0] != '\0') {
            String fpath = g_configs[i].imgPath;
            if(!fpath.startsWith("/")) fpath = "/" + fpath;
            
            if (LittleFS.exists(fpath)) {
                lv_obj_t *img = lv_image_create(btn);
                char full_path[64];
                sprintf(full_path, "L:%s", fpath.c_str());
                lv_image_set_src(img, full_path);
                
                // Scale image if grid is dense
                if (g_cols > 4 || g_rows > 3) lv_obj_set_size(img, 48, 48);
                else lv_obj_set_size(img, 64, 64);
                
                icon_or_img_present = true;
            }
        }

        if (!icon_or_img_present && g_configs[i].icon[0] != '\0') {
            lv_obj_t *icon = lv_label_create(btn);
            lv_label_set_text(icon, g_configs[i].icon);
            // Smaller font if grid is dense
            if (g_cols > 4) lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, 0);
            else lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
            icon_or_img_present = true;
        }

        // Label - Only create if not empty, to allow centering of icon/image
        if (g_configs[i].label[0] != '\0') {
            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text(label, g_configs[i].label);
            if (g_cols > 4) lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
            else lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        }
        
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }

    // Slider Row
    lv_obj_t *slider = lv_slider_create(grid);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_set_grid_cell(slider, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, g_rows, 1);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // WiFi Status Label
    lv_obj_t *wlbl = lv_label_create(grid);
    String wtxt = "\xEF\x87\xAB " + g_ip_addr; // WIFI icon
    lv_label_set_text(wlbl, wtxt.c_str());
    lv_obj_set_grid_cell(wlbl, LV_GRID_ALIGN_CENTER, 1, (g_cols > 2 ? g_cols - 2 : 1), LV_GRID_ALIGN_CENTER, g_rows, 1);
    
    // Settings Button
    lv_obj_t *set_btn = lv_btn_create(grid);
    lv_obj_set_grid_cell(set_btn, LV_GRID_ALIGN_STRETCH, g_cols - 1, 1, LV_GRID_ALIGN_STRETCH, g_rows, 1);
    lv_obj_t *set_label = lv_label_create(set_btn);
    lv_label_set_text(set_label, "\xEF\x80\x93" " Config"); // SETTINGS
    lv_obj_add_event_cb(set_btn, settings_btn_cb, LV_EVENT_CLICKED, NULL);
}

// ==========================================
// UI - SETTINGS LIST
// ==========================================
static void create_settings_ui() {
    g_settings_screen = lv_obj_create(NULL);
    lv_scr_load(g_settings_screen);
    lv_obj_set_style_bg_color(g_settings_screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(g_settings_screen);
    lv_label_set_text(title, "Settings - Customization");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *list = lv_list_create(g_settings_screen);
    lv_obj_set_size(list, 600, 360); // Slightly taller
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 45);

    lv_obj_t *bg_btn = lv_list_add_btn(list, "\xEF\x80\xBE", "Global Background Color"); // IMAGE
    lv_obj_add_event_cb(bg_btn, settings_bg_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *grid_btn = lv_list_add_btn(list, "\xEF\x80\x8A", "Grid Layout Size"); // THUMBNAILS/GRID
    lv_obj_add_event_cb(grid_btn, settings_grid_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *os_btn = lv_list_add_btn(list, "\xEF\x84\xb9", "Target OS (Win/Mac)"); // DESKTOP
    lv_obj_add_event_cb(os_btn, settings_os_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *wifi_btn = lv_list_add_btn(list, "\xEF\x87\xAB", "WiFi Setup"); // WIFI
    lv_obj_add_event_cb(wifi_btn, settings_wifi_btn_cb, LV_EVENT_CLICKED, NULL);

    int btn_count = g_rows * g_cols;
    for (int i = 0; i < btn_count; i++) {
        char buf[32];
        sprintf(buf, "Button %d: %s", (i+1), g_configs[i].label);
        lv_obj_t *btn = lv_list_add_btn(list, "\xEF\x8C\x84", buf); // EDIT
        lv_obj_add_event_cb(btn, edit_btn_select_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }

    lv_obj_t *back = lv_btn_create(g_settings_screen);
    lv_obj_set_size(back, 140, 50);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t *lbl = lv_label_create(back);
    lv_label_set_text(lbl, "\xEF\x81\x93" " Back"); // LEFT
    lv_obj_add_event_cb(back, back_to_main_cb, LV_EVENT_CLICKED, NULL);
}

// ==========================================
// UI - WIFI SETUP SCREEN
// ==========================================
struct WifiUIData {
    lv_obj_t* ta_ssid;
    lv_obj_t* ta_pass;
};
static WifiUIData g_wifi_data;

static void create_wifi_ui() {
    g_wifi_screen = lv_obj_create(NULL);
    lv_scr_load(g_wifi_screen);
    lv_obj_set_style_bg_color(g_wifi_screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(g_wifi_screen);
    lv_label_set_text(title, "WiFi Configuration");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *l1 = lv_label_create(g_wifi_screen);
    lv_label_set_text(l1, "SSID:");
    lv_obj_align(l1, LV_ALIGN_TOP_LEFT, 20, 50);
    g_wifi_data.ta_ssid = lv_textarea_create(g_wifi_screen);
    lv_textarea_set_one_line(g_wifi_data.ta_ssid, true);
    lv_obj_set_size(g_wifi_data.ta_ssid, 350, 40);
    lv_obj_align(g_wifi_data.ta_ssid, LV_ALIGN_TOP_LEFT, 20, 70);
    lv_textarea_set_text(g_wifi_data.ta_ssid, g_wifi_ssid);

    lv_obj_t *l2 = lv_label_create(g_wifi_screen);
    lv_label_set_text(l2, "Password:");
    lv_obj_align(l2, LV_ALIGN_TOP_LEFT, 20, 120);
    g_wifi_data.ta_pass = lv_textarea_create(g_wifi_screen);
    lv_textarea_set_one_line(g_wifi_data.ta_pass, true);
    lv_textarea_set_password_mode(g_wifi_data.ta_pass, true);
    lv_obj_set_size(g_wifi_data.ta_pass, 350, 40);
    lv_obj_align(g_wifi_data.ta_pass, LV_ALIGN_TOP_LEFT, 20, 140);
    lv_textarea_set_text(g_wifi_data.ta_pass, g_wifi_pass);

    lv_obj_t *kb = lv_keyboard_create(g_wifi_screen);
    lv_keyboard_set_textarea(kb, g_wifi_data.ta_ssid);
    lv_obj_set_size(kb, 780, 240);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    lv_obj_add_event_cb(g_wifi_data.ta_ssid, kb_focus_cb, LV_EVENT_FOCUSED, kb);
    lv_obj_add_event_cb(g_wifi_data.ta_pass, kb_focus_cb, LV_EVENT_FOCUSED, kb);

    lv_obj_t *save = lv_btn_create(g_wifi_screen);
    lv_obj_set_size(save, 160, 50);
    lv_obj_align(save, LV_ALIGN_TOP_RIGHT, -20, 70);
    lv_obj_t *sl = lv_label_create(save);
    lv_label_set_text(sl, "\xEF\x83\x87" " Save & Connect");
    lv_obj_add_event_cb(save, save_wifi_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel = lv_btn_create(g_wifi_screen);
    lv_obj_set_size(cancel, 120, 50);
    lv_obj_align(cancel, LV_ALIGN_TOP_RIGHT, -20, 130);
    lv_obj_t *cl = lv_label_create(cancel);
    lv_label_set_text(cl, "\xEF\x80\x8D" " Cancel");
    lv_obj_add_event_cb(cancel, back_to_main_cb, LV_EVENT_CLICKED, NULL);
}

// (Moved up)

struct EditUIData {
    lv_obj_t* dd_icon;
    lv_obj_t* dd_type;
    lv_obj_t* ta_label;
    lv_obj_t* ta_value;
    lv_obj_t* dd_img;
};

static EditUIData g_edit_data;

static void create_edit_ui(uint8_t idx) {
    if (!g_editing_bg) g_editing_idx = idx;
    g_edit_screen = lv_obj_create(NULL);
    lv_scr_load(g_edit_screen);
    lv_obj_set_style_bg_color(g_edit_screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(g_edit_screen);
    if (g_editing_bg) lv_label_set_text(title, "Editing Global Background");
    else lv_label_set_text_fmt(title, "Editing Button %d", (idx + 1));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    uint32_t curr_color = g_editing_bg ? g_bg_color : g_configs[idx].color;

    if (!g_editing_bg) {
        // Label Field
        lv_obj_t *l1 = lv_label_create(g_edit_screen);
        lv_label_set_text(l1, "Label:");
        lv_obj_align(l1, LV_ALIGN_TOP_LEFT, 20, 35);
        g_edit_data.ta_label = lv_textarea_create(g_edit_screen);
        lv_textarea_set_one_line(g_edit_data.ta_label, true);
        lv_obj_set_size(g_edit_data.ta_label, 180, 40);
        lv_obj_align(g_edit_data.ta_label, LV_ALIGN_TOP_LEFT, 20, 55);
        lv_textarea_set_text(g_edit_data.ta_label, g_configs[idx].label);

        // Icon Selector
        lv_obj_t *li = lv_label_create(g_edit_screen);
        lv_label_set_text(li, "Icon:");
        lv_obj_align(li, LV_ALIGN_TOP_LEFT, 220, 35);
        g_edit_data.dd_icon = lv_dropdown_create(g_edit_screen);
        lv_obj_set_size(g_edit_data.dd_icon, 180, 40);
        lv_dropdown_set_options(g_edit_data.dd_icon, "None\nOK\nClose\nCopy\nPaste\nCut\nPlay\nPause\nMute\nLoop\nPlus\nSave\nEdit\nFile\nDir\nWarning");
        lv_obj_align(g_edit_data.dd_icon, LV_ALIGN_TOP_LEFT, 220, 55);
        lv_dropdown_set_selected(g_edit_data.dd_icon, get_index_by_symbol(g_configs[idx].icon));

        // Type Selector
        lv_obj_t *l2 = lv_label_create(g_edit_screen);
        lv_label_set_text(l2, "Action:");
        lv_obj_align(l2, LV_ALIGN_TOP_LEFT, 20, 105);
        g_edit_data.dd_type = lv_dropdown_create(g_edit_screen);
        lv_obj_set_size(g_edit_data.dd_type, 180, 40);
        lv_dropdown_set_options(g_edit_data.dd_type, "Launch App\nMedia Key\nKey Ctrl+");
        lv_obj_align(g_edit_data.dd_type, LV_ALIGN_TOP_LEFT, 20, 125);
        lv_dropdown_set_selected(g_edit_data.dd_type, g_configs[idx].type);

        // Value Field
        lv_obj_t *l3 = lv_label_create(g_edit_screen);
        lv_label_set_text(l3, "Cmd/Key:");
        lv_obj_align(l3, LV_ALIGN_TOP_LEFT, 220, 105);
        g_edit_data.ta_value = lv_textarea_create(g_edit_screen);
        lv_textarea_set_one_line(g_edit_data.ta_value, true);
        lv_textarea_set_max_length(g_edit_data.ta_value, 127);
        lv_obj_set_size(g_edit_data.ta_value, 180, 40);
        lv_obj_align(g_edit_data.ta_value, LV_ALIGN_TOP_LEFT, 220, 125);
        lv_textarea_set_text(g_edit_data.ta_value, g_configs[idx].value);

        // Custom Image Selector
        lv_obj_t *li2 = lv_label_create(g_edit_screen);
        lv_label_set_text(li2, "Custom Image:");
        lv_obj_align(li2, LV_ALIGN_TOP_LEFT, 120, 175);
        g_edit_data.dd_img = lv_dropdown_create(g_edit_screen);
        lv_obj_set_size(g_edit_data.dd_img, 180, 40);
        lv_obj_align(g_edit_data.dd_img, LV_ALIGN_TOP_LEFT, 120, 195);

        String opts = "Ninguno";
        File root = LittleFS.open("/");
        File f = root.openNextFile();
        int sel_idx = 0;
        int current_f = 1;
        while(f){
            String fname = f.name();
            if (!fname.startsWith("/")) fname = "/" + fname;
            opts += "\n" + fname.substring(1); // Show name without slash in dropdown
            if (fname == g_configs[idx].imgPath) sel_idx = current_f;
            f = root.openNextFile();
            current_f++;
        }
        lv_dropdown_set_options(g_edit_data.dd_img, opts.c_str());
        lv_dropdown_set_selected(g_edit_data.dd_img, sel_idx);
    }

    // RGB Sliders & Preview - Fixed Position to avoid overlap
    int panel_x = 450;
    auto create_rgb_slider = [&](lv_obj_t** slider, int y, uint8_t val, lv_color_t color) {
        *slider = lv_slider_create(g_edit_screen);
        lv_obj_set_size(*slider, 200, 15);
        lv_obj_align(*slider, LV_ALIGN_TOP_LEFT, panel_x, y);
        lv_slider_set_range(*slider, 0, 255);
        lv_slider_set_value(*slider, val, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(*slider, color, LV_PART_KNOB);
        lv_obj_add_event_cb(*slider, color_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    };

    create_rgb_slider(&g_slider_r, 55, (curr_color >> 16) & 0xFF, lv_color_hex(0xFF0000));
    create_rgb_slider(&g_slider_g, 95, (curr_color >> 8) & 0xFF, lv_color_hex(0x00FF00));
    create_rgb_slider(&g_slider_b, 135, (curr_color >> 0) & 0xFF, lv_color_hex(0x0000FF));

    g_preview = lv_obj_create(g_edit_screen);
    lv_obj_set_size(g_preview, 100, 100);
    lv_obj_align(g_preview, LV_ALIGN_TOP_LEFT, panel_x + 220, 50);
    lv_obj_set_style_bg_color(g_preview, lv_color_hex(curr_color), LV_PART_MAIN);

    if (!g_editing_bg) {
        lv_obj_t *kb = lv_keyboard_create(g_edit_screen);
        lv_keyboard_set_textarea(kb, g_edit_data.ta_label);
        lv_obj_set_size(kb, 780, 220); // slightly shorter to fit buttons
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -5);
        
        lv_obj_add_event_cb(g_edit_data.ta_label, kb_focus_cb, LV_EVENT_FOCUSED, kb);
        lv_obj_add_event_cb(g_edit_data.ta_value, kb_focus_cb, LV_EVENT_FOCUSED, kb);
    }

    lv_obj_t *save = lv_btn_create(g_edit_screen);
    lv_obj_set_size(save, 140, 50);
    lv_obj_align(save, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
    lv_obj_t *sl = lv_label_create(save);
    lv_label_set_text(sl, "\xEF\x83\x87" " Save"); // SAVE
    lv_obj_add_event_cb(save, save_edit_cb, LV_EVENT_CLICKED, &g_edit_data);

    lv_obj_t *cancel = lv_btn_create(g_edit_screen);
    lv_obj_set_size(cancel, 140, 50);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    lv_obj_t *cl = lv_label_create(cancel);
    lv_label_set_text(cl, "\xEF\x80\x8D" " Cancel"); // CLOSE
    lv_obj_add_event_cb(cancel, back_to_main_cb, LV_EVENT_CLICKED, NULL);
}

// ==========================================
// CALLBACKS
// ==========================================
static void btn_event_cb(lv_event_t *e) {
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    StreamDeckApp::handle_button(idx);
}

static void slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = (lv_obj_t*)lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    set_brightness((uint8_t)val);
}

static void color_slider_cb(lv_event_t *e) {
    lv_color_t c;
    c.red = (uint8_t)lv_slider_get_value(g_slider_r);
    c.green = (uint8_t)lv_slider_get_value(g_slider_g);
    c.blue = (uint8_t)lv_slider_get_value(g_slider_b);
    lv_obj_set_style_bg_color(g_preview, c, LV_PART_MAIN);
}

static void kb_focus_cb(lv_event_t *e) {
    lv_obj_t *ta = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t *kb = (lv_obj_t*)lv_event_get_user_data(e);
    lv_keyboard_set_textarea(kb, ta);
}

static void settings_bg_btn_cb(lv_event_t* e) {
    g_editing_bg = true;
    create_edit_ui(0);
}

static void settings_wifi_btn_cb(lv_event_t* e) {
    create_wifi_ui();
}

static void save_wifi_cb(lv_event_t *e) {
    strncpy(g_wifi_ssid, lv_textarea_get_text(g_wifi_data.ta_ssid), 31);
    strncpy(g_wifi_pass, lv_textarea_get_text(g_wifi_data.ta_pass), 63);
    
    save_settings();
    WiFi.disconnect();
    WiFi.begin(g_wifi_ssid, g_wifi_pass);
    
    lv_scr_load(g_main_screen);
    create_main_ui();
}

static void settings_btn_cb(lv_event_t *e) {
    create_settings_ui();
}

static void back_to_main_cb(lv_event_t *e) {
    g_editing_bg = false;
    lv_scr_load(g_main_screen);
    create_main_ui();
}

static void edit_btn_select_cb(lv_event_t *e) {
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    g_editing_bg = false;
    create_edit_ui(idx);
}

static void save_edit_cb(lv_event_t *e) {
    EditUIData* data = (EditUIData*)lv_event_get_user_data(e);
    
    uint8_t r = (uint8_t)lv_slider_get_value(g_slider_r);
    uint8_t g = (uint8_t)lv_slider_get_value(g_slider_g);
    uint8_t b = (uint8_t)lv_slider_get_value(g_slider_b);
    uint32_t hex = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;

    if (g_editing_bg) {
        g_bg_color = hex;
    } else {
        memset(g_configs[g_editing_idx].label, 0, 16);
        memset(g_configs[g_editing_idx].value, 0, 128);
        
        if (data) {
            strncpy(g_configs[g_editing_idx].label, lv_textarea_get_text(data->ta_label), 15);
            strncpy(g_configs[g_editing_idx].value, lv_textarea_get_text(data->ta_value), 127);
            g_configs[g_editing_idx].type = (uint8_t)lv_dropdown_get_selected(data->dd_type);
            const char* sym = get_symbol_by_index(lv_dropdown_get_selected(data->dd_icon));
            strncpy(g_configs[g_editing_idx].icon, sym, 7);

            char buf[32];
            lv_dropdown_get_selected_str(data->dd_img, buf, sizeof(buf));
            if (strcmp(buf, "Ninguno") == 0) {
                g_configs[g_editing_idx].imgPath[0] = '\0';
            } else {
                String val = buf;
                if (!val.startsWith("/")) val = "/" + val;
                strncpy(g_configs[g_editing_idx].imgPath, val.c_str(), 31);
            }
        }
        
        g_configs[g_editing_idx].color = hex;
    }
    
    save_settings();
    g_editing_bg = false;
    lv_scr_load(g_main_screen);
    create_main_ui();
}

static void grid_select_cb(lv_event_t *e) {
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    char buf[16];
    const char * txt = lv_list_get_button_text(lv_obj_get_parent(obj), obj);
    if(txt) strncpy(buf, txt, sizeof(buf));
    else buf[0] = '\0';
    
    if (strcmp(buf, "2x2") == 0) { g_cols = 2; g_rows = 2; }
    else if (strcmp(buf, "3x2") == 0) { g_cols = 3; g_rows = 2; }
    else if (strcmp(buf, "3x3") == 0) { g_cols = 3; g_rows = 3; }
    else if (strcmp(buf, "4x3") == 0) { g_cols = 4; g_rows = 3; }
    else if (strcmp(buf, "5x3") == 0) { g_cols = 5; g_rows = 3; }
    
    save_settings();
    lv_scr_load(g_main_screen);
    create_main_ui();
}

static void settings_grid_btn_cb(lv_event_t* e) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(g_bg_color), LV_PART_MAIN);
    
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Select Grid Layout");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *list = lv_list_create(screen);
    lv_obj_set_size(list, 400, 320);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);
    
    const char* opts[] = {"2x2", "3x2", "3x3", "4x3", "5x3"};
    for(int i=0; i<5; i++) {
        lv_obj_t *btn = lv_list_add_btn(list, "\xEF\x80\x8A", opts[i]);
        lv_obj_add_event_cb(btn, grid_select_cb, LV_EVENT_CLICKED, NULL);
    }
    
    lv_obj_t *back = lv_btn_create(screen);
    lv_obj_set_size(back, 120, 45);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_t *lbl = lv_label_create(back);
    lv_label_set_text(lbl, "Cancel");
    lv_obj_add_event_cb(back, settings_btn_cb, LV_EVENT_CLICKED, NULL);
}

static void os_select_cb(lv_event_t *e) {
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    char buf[16];
    const char * txt = lv_list_get_button_text(lv_obj_get_parent(obj), obj);
    if(txt) strncpy(buf, txt, sizeof(buf));
    else buf[0] = '\0';
    
    if (strcmp(buf, "Windows") == 0) g_target_os = 0;
    else if (strcmp(buf, "macOS") == 0) g_target_os = 1;
    
    save_settings(false); // Only save the OS preference, DON'T overwrite buttons
    load_settings();     // Reload Buttons for the target OS
    lv_scr_load(g_main_screen);
    create_main_ui();
}

static void settings_os_btn_cb(lv_event_t* e) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(g_bg_color), LV_PART_MAIN);
    
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Select Target OS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *list = lv_list_create(screen);
    lv_obj_set_size(list, 400, 320);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);
    
    const char* opts[] = {"Windows", "macOS"};
    for(int i=0; i<2; i++) {
        lv_obj_t *btn = lv_list_add_btn(list, "\xEF\x84\xb9", opts[i]);
        lv_obj_add_event_cb(btn, os_select_cb, LV_EVENT_CLICKED, NULL);
    }
    
    lv_obj_t *back = lv_btn_create(screen);
    lv_obj_set_size(back, 120, 45);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_t *lbl = lv_label_create(back);
    lv_label_set_text(lbl, "Cancel");
    lv_obj_add_event_cb(back, settings_btn_cb, LV_EVENT_CLICKED, NULL);
}


