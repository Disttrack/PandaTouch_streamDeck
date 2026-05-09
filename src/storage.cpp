#include "storage.h"
#include <Preferences.h>
#include <LittleFS.h>
#include <WiFi.h>

struct LegacyButtonConfig {
    char label[16];
    char value[128];
    uint8_t type;
    uint32_t color;
    char icon[8];
    char imgPath[32];
};

ButtonConfig g_configs[MAX_BUTTONS];
uint32_t g_bg_color = 0x121212;
uint8_t g_rows = 3;
uint8_t g_cols = 3;
uint8_t g_target_os = OS_WINDOWS;
char g_wifi_ssid[32] = "";
char g_wifi_pass[64] = "";
uint8_t g_kb_lang = LANG_US;
uint8_t g_brightness = 50;
String g_wifi_status = "Disconnected";
String g_ip_addr = "0.0.0.0";

static Preferences preferences;

void init_storage() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
    } else {
        Serial.println("LittleFS Mounted Successfully. Files:");
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        while (file) {
            file = root.openNextFile();
        }
    }
}

void load_settings() {
    preferences.begin("deck", false);
    g_rows = preferences.getUChar("rows", 3);
    g_cols = preferences.getUChar("cols", 3);
    g_target_os = preferences.getUChar("os", 0);
    g_kb_lang = preferences.getUChar("lang", 0);
    g_bg_color = preferences.getUInt("bg", 0x121212);
    g_brightness = preferences.getUChar("bright", 50);

    if (g_bg_color == 0x000000) g_bg_color = 0x121212;
    if (g_brightness < 1) g_brightness = 50;
    if (g_brightness > 100) g_brightness = 100;
    if (g_rows < 1 || g_rows > 5) g_rows = 3;
    if (g_cols < 1 || g_cols > 5) g_cols = 3;

    const char* win_file = "/win_btns.bin";
    const char* mac_file = "/mac_btns.bin";

    auto migrate_file = [](const char* path) {
        File f = LittleFS.open(path, "r");
        if (!f) return;
        size_t size = f.size();
        if (size == sizeof(LegacyButtonConfig) * MAX_BUTTONS) {
            Serial.printf("STORAGE: Migrating %s to new format...\n", path);
            LegacyButtonConfig old_btns[MAX_BUTTONS];
            f.read((uint8_t*)old_btns, sizeof(old_btns));
            f.close();

            ButtonConfig new_btns[MAX_BUTTONS];
            memset(new_btns, 0, sizeof(new_btns));
            for (int i = 0; i < MAX_BUTTONS; i++) {
                strncpy(new_btns[i].label, old_btns[i].label, 15);
                strncpy(new_btns[i].value, old_btns[i].value, 127);
                new_btns[i].type = old_btns[i].type;
                new_btns[i].color = old_btns[i].color;
                strncpy(new_btns[i].icon, old_btns[i].icon, 7);
                strncpy(new_btns[i].imgPath, old_btns[i].imgPath, 31);
            }

            f = LittleFS.open(path, "w");
            if (f) {
                f.write((uint8_t*)new_btns, sizeof(new_btns));
                f.close();
                Serial.println("STORAGE: Migration successful.");
            }
        } else {
            f.close();
        }
    };

    migrate_file(win_file);
    migrate_file(mac_file);

    if (!preferences.getBool("init_os_v4", false)) {
        bool has_win = LittleFS.exists(win_file);
        bool has_mac = LittleFS.exists(mac_file);

        if (has_win && has_mac) {
            preferences.putBool("init_os_v4", true);
            Serial.println("STORAGE: init_os_v4 flag recovered (bin files exist).");
        } else {
            Serial.println("Initial Profile Setup (v4 LittleFS): Migrating...");
            has_win = false; has_mac = false;

            auto set_defaults = []() {
                for (int i = 0; i < MAX_BUTTONS; i++) {
                    memset(&g_configs[i], 0, sizeof(ButtonConfig));
                    g_configs[i].color = 0x333333;
                    strncpy(g_configs[i].label, "Button", 15);
                }
            };

            set_defaults();
            if (preferences.getBytes("w_pA", &g_configs[0], 10 * sizeof(ButtonConfig)) > 0) {
                preferences.getBytes("w_pB", &g_configs[10], 10 * sizeof(ButtonConfig));
            } else {
                for (int i = 0; i < MAX_BUTTONS; i++) {
                    char k1[8], k2[8];
                    sprintf(k1, "b%d", i);
                    sprintf(k2, "wb%d", i);
                    if (preferences.getBytes(k2, &g_configs[i], sizeof(ButtonConfig)) == 0)
                        preferences.getBytes(k1, &g_configs[i], sizeof(ButtonConfig));
                }
            }
            {
                File f = LittleFS.open(win_file, "w");
                if (f) {
                    f.write((uint8_t*)g_configs, sizeof(g_configs));
                    f.close();
                }
            }

            set_defaults();
            if (preferences.getBytes("m_pA", &g_configs[0], 10 * sizeof(ButtonConfig)) > 0) {
                preferences.getBytes("m_pB", &g_configs[10], 10 * sizeof(ButtonConfig));
            } else {
                for (int i = 0; i < MAX_BUTTONS; i++) {
                    char k3[8];
                    sprintf(k3, "mb%d", i);
                    preferences.getBytes(k3, &g_configs[i], sizeof(ButtonConfig));
                }
            }
            {
                File f = LittleFS.open(mac_file, "w");
                if (f) {
                    f.write((uint8_t*)g_configs, sizeof(g_configs));
                    f.close();
                }
            }

            preferences.putBool("init_os_v4", true);
            Serial.println("STORAGE: Migration to LittleFS files complete.");
        }
    }

    const char* active_file = (g_target_os == OS_WINDOWS ? win_file : mac_file);
    File f = LittleFS.open(active_file, "r");
    if (f) {
        size_t read = f.read((uint8_t*)g_configs, sizeof(g_configs));
        f.close();
        if (read != sizeof(g_configs)) {
            Serial.println("FAIL (size mismatch)");
            goto load_defaults;
        }
        Serial.println("OK");
    } else {
        Serial.println("NOT FOUND");
    load_defaults:
        for (int i = 0; i < MAX_BUTTONS; i++) {
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

void save_settings(bool saveButtons) {
    preferences.begin("deck", false);
    preferences.putUInt("bg", g_bg_color);
    preferences.putUChar("bright", g_brightness);
    preferences.putUChar("rows", g_rows);
    preferences.putUChar("cols", g_cols);
    preferences.putUChar("os", g_target_os);
    preferences.putUChar("lang", g_kb_lang);
    preferences.putString("wssid", g_wifi_ssid);
    preferences.putString("wpass", g_wifi_pass);
    preferences.end();

    if (saveButtons) {
        const char* active_file = (g_target_os == OS_WINDOWS ? "/win_btns.bin" : "/mac_btns.bin");
        File f = LittleFS.open(active_file, "w");
        if (f) {
            f.write((uint8_t*)g_configs, sizeof(g_configs));
            f.close();
        } else {
            Serial.printf("STORAGE ERROR: Failed to open %s for writing\n", active_file);
        }
    }
}
