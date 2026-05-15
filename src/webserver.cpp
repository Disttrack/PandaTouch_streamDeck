#include "webserver.h"
#include "storage.h"
#include "l10n.h"
#include "ui_main.h"
#include "webserver_html.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <LittleFS.h>
#include <base64.h>
#include <esp_task_wdt.h>
#include <vector>

AsyncWebServer server(80);

static bool webserver_started = false;

static String escape_json(String s) {
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    s.replace("\n", "\\n");
    s.replace("\r", "\\r");
    s.replace("\t", "\\t");
    return s;
}

static uint32_t parse_color(String hex) {
    if (hex.startsWith("#")) hex = hex.substring(1);
    return strtol(hex.c_str(), NULL, 16);
}

static String find_icon_name(const char* code) {
    if (!code || code[0] == '\0') return "None";
    for (int j = 0; j < 20; j++) {
        if (strcmp(code, g_sym_codes[j]) == 0) return g_sym_names[j];
    }
    return "None";
}

static void build_config_json(AsyncWebServerRequest* request) {
    JsonDocument doc;
    char buf[16];

    snprintf(buf, sizeof(buf), "%06X", g_bg_color);
    doc["bg"] = buf;
    doc["rows"] = g_rows;
    doc["cols"] = g_cols;
    doc["os"] = g_target_os;
    doc["lang"] = g_kb_lang;

    JsonArray btns = doc["buttons"].to<JsonArray>();
    for (int i = 0; i < MAX_BUTTONS; i++) {
        JsonObject b = btns.add<JsonObject>();
        b["label"] = g_configs[i].label;
        b["value"] = g_configs[i].value;
        b["type"] = g_configs[i].type;
        snprintf(buf, sizeof(buf), "%06X", g_configs[i].color);
        b["color"] = buf;
        b["icon"] = find_icon_name(g_configs[i].icon);
        b["img"] = g_configs[i].imgPath;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

static void build_l10n_json(AsyncWebServerRequest* request) {
    const L10n* l = get_l10n();
    JsonDocument doc;

    doc["dash_title"] = l->dash_title;
    doc["kb_label"] = l->kb_label;
    doc["os_label"] = l->os_label;
    doc["grid_label"] = l->grid_label;
    doc["bg_label"] = l->bg_label;
    doc["btn_config"] = l->btn_config;
    doc["btn_name_ph"] = l->btn_name_ph;
    doc["btn_cmd_ph"] = l->btn_cmd_ph;
    doc["type_app"] = l->type_app;
    doc["type_media"] = l->type_media;
    doc["type_basic"] = l->type_basic;
    doc["type_adv"] = l->type_adv;
    doc["save_changes"] = l->save_changes;
    doc["library"] = l->library;
    doc["upload"] = l->upload;
    doc["backup_title"] = l->backup_title;
    doc["backup_btn"] = l->backup_btn;
    doc["restore_btn"] = l->restore_btn;
    doc["firmware_title"] = l->firmware_title;
    doc["firmware_info"] = l->firmware_info;
    doc["update_btn"] = l->update_btn;
    doc["updating_msg"] = l->updating_msg;
    doc["confirm_restore"] = l->confirm_restore;
    doc["restore_ok"] = l->restore_ok;
    doc["config_saved"] = l->config_saved;
    doc["delete_file_confirm"] = l->delete_file_confirm;
    doc["update_firmware_confirm"] = l->update_firmware_confirm;
    doc["none"] = l->none;
    doc["basic_combo_desc"] = l->basic_combo_desc;
    doc["button_label"] = l->button_label;
    doc["select_key_ph"] = l->select_key_ph;
    doc["color_title"] = l->color_title;
    doc["icon_title"] = l->icon_title;
    doc["image_title"] = l->image_title;
    doc["please_select_bin"] = "Please select a .bin file";
    doc["invalid_file_size"] = "Invalid file size: ";
    doc["must_be_between_100kb_3mb"] = "Must be between 100KB and 3MB.";
    doc["uploading_firmware"] = "Uploading firmware (";
    doc["uploading"] = "Uploading";
    doc["error"] = "Error";
    doc["update_successful"] = "Update successful! Device restarting...";
    doc["connection_failed"] = "Connection failed. Please try again.";
    doc["timeout_msg"] = "Timeout - Device may still be updating. Wait 30 seconds before retrying.";
    doc["settings_title"] = l->settings_title;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void check_wifi_status() {
    static bool was_connected = false;
    static unsigned long last_wifi_check = 0;

    if (millis() - last_wifi_check < 2000) return;
    last_wifi_check = millis();

    bool is_connected = (WiFi.status() == WL_CONNECTED);

    if (is_connected != was_connected) {
        if (is_connected) {
            g_wifi_status = "Connected";
            g_ip_addr = WiFi.localIP().toString();
            Serial.print("WiFi Connected! IP: ");
            Serial.println(g_ip_addr);
            init_webserver();
        } else {
            g_wifi_status = "Disconnected";
            g_ip_addr = "0.0.0.0";
            Serial.println("WiFi Disconnected.");
        }
        was_connected = is_connected;
        if (g_wifi_label) {
            String wtxt = "\xEF\x87\xAB " + g_ip_addr;
            lv_label_set_text(g_wifi_label, wtxt.c_str());
        }
    }
}

void init_webserver() {
    if (webserver_started) return;

    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        build_config_json(request);
    });

    server.on("/api/l10n", HTTP_GET, [](AsyncWebServerRequest* request) {
        build_l10n_json(request);
    });

    server.on("/api/save", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("bg", true)) g_bg_color = parse_color(request->getParam("bg", true)->value());
        if (request->hasParam("rows", true)) { g_rows = request->getParam("rows", true)->value().toInt(); if (g_rows < 1 || g_rows > 5) g_rows = 3; }
        if (request->hasParam("cols", true)) { g_cols = request->getParam("cols", true)->value().toInt(); if (g_cols < 1 || g_cols > 5) g_cols = 3; }
        if (request->hasParam("os", true)) g_target_os = request->getParam("os", true)->value().toInt();
        if (request->hasParam("lang", true)) g_kb_lang = request->getParam("lang", true)->value().toInt();

        uint32_t changed_mask = 0;
        for (int i = 0; i < MAX_BUTTONS; i++) {
            String p = "b" + String(i);
            bool changed = false;

            memset(g_configs[i].label, 0, 16);
            memset(g_configs[i].value, 0, 256);
            memset(g_configs[i].icon, 0, 8);
            memset(g_configs[i].imgPath, 0, 32);

            if (request->hasParam(p + "l", true)) {
                String label = request->getParam(p + "l", true)->value();
                strncpy(g_configs[i].label, label.c_str(), 15);
                g_configs[i].label[15] = '\0';
                changed = true;
            }

            if (request->hasParam(p + "v", true)) {
                String value = request->getParam(p + "v", true)->value();
                strncpy(g_configs[i].value, value.c_str(), 255);
                g_configs[i].value[255] = '\0';
                changed = true;
            }

            if (request->hasParam(p + "t", true)) {
                g_configs[i].type = request->getParam(p + "t", true)->value().toInt();
                changed = true;
            }

            if (request->hasParam(p + "c", true)) {
                g_configs[i].color = parse_color(request->getParam(p + "c", true)->value());
                changed = true;
            }

            if (request->hasParam(p + "icon", true)) {
                String iconName = request->getParam(p + "icon", true)->value();
                bool found = false;
                for (int j = 0; j < 20; j++) {
                    if (iconName == g_sym_names[j]) {
                        const char* sym = g_sym_codes[j];
                        size_t sym_len = strlen(sym);
                        if (sym_len > 0 && sym_len < 8) {
                            strncpy(g_configs[i].icon, sym, sym_len);
                            g_configs[i].icon[sym_len] = '\0';
                        } else if (sym_len == 0) {
                            g_configs[i].icon[0] = '\0';
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    g_configs[i].icon[0] = '\0';
                }
                changed = true;
            }

            if (request->hasParam(p + "i", true)) {
                String val = request->getParam(p + "i", true)->value();
                if (val.length() > 0 && val != String(get_l10n()->none)) {
                    if (!val.startsWith("/")) val = "/" + val;
                    strncpy(g_configs[i].imgPath, val.c_str(), 31);
                    g_configs[i].imgPath[31] = '\0';
                }
                changed = true;
            }

            if (changed && i < 32) changed_mask |= (1u << i);
        }

        bool isOSSwitch = (request->hasParam("os", true) && request->params() <= 2);

        if (!set_pending_redundant_write(true)) {
            save_settings(!isOSSwitch);
            load_settings();
            set_pending_redundant_write(false);
        } else {
            save_settings(!isOSSwitch);
        }

        g_dirty_buttons_mask = changed_mask;
        g_dirty_bg = request->hasParam("bg", true);
        g_dirty_layout = request->hasParam("rows", true) || request->hasParam("cols", true);
        g_pending_ui_update = true;

        Serial.println("WEB API: Configuration saved successfully");
        request->send(200, "text/plain", "OK");
    });

    server.on("/api/backup", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Build metadata-only JSON (no asset blobs) to keep JsonDocument small
        JsonDocument doc;
        char buf[16];

        snprintf(buf, sizeof(buf), "%06X", g_bg_color);
        doc["bg"] = buf;
        doc["rows"] = g_rows;
        doc["cols"] = g_cols;
        doc["os"] = g_target_os;
        doc["lang"] = g_kb_lang;
        doc["wifi_ssid"] = g_wifi_ssid;

        auto save_btns_to_json = [&](const char* path, const char* key) {
            ButtonConfig btns[MAX_BUTTONS];
            File f = LittleFS.open(path, "r");
            if (f) {
                f.read((uint8_t*)btns, sizeof(btns));
                f.close();
            }
            JsonArray arr = doc[key].to<JsonArray>();
            for (int i = 0; i < MAX_BUTTONS; i++) {
                JsonObject b = arr.add<JsonObject>();
                b["label"] = btns[i].label;
                b["value"] = btns[i].value;
                b["type"] = btns[i].type;
                snprintf(buf, sizeof(buf), "%06X", btns[i].color);
                b["color"] = buf;
                b["icon"] = btns[i].icon;
                b["img"] = btns[i].imgPath;
            }
        };

        save_btns_to_json("/win_btns.bin", "win_btns");
        save_btns_to_json("/mac_btns.bin", "mac_btns");

        // Serialize metadata to string first
        String output;
        serializeJson(doc, output);
        doc.clear();  // free arena memory

        // Append assets one at a time to avoid keeping all Base64 in RAM
        output += ",\"assets\":{";
        File root = LittleFS.open("/");
        bool firstAsset = true;
        if (root) {
            File assetFile = root.openNextFile();
            while (assetFile) {
                String name = String(assetFile.name());
                if (!assetFile.isDirectory() &&
                    name != "win_btns.bin" &&
                    name != "mac_btns.bin" &&
                    !name.startsWith("._")) {

                    size_t size = assetFile.size();
                    uint8_t* buf2 = (uint8_t*)malloc(size);
                    if (buf2) {
                        assetFile.read(buf2, size);
                        String encoded = base64::encode(buf2, size);
                        free(buf2);

                        if (!firstAsset) output += ",";
                        firstAsset = false;
                        output += "\"" + name + "\":\"" + encoded + "\"";
                        // encoded goes out of scope → frees asset's Base64 before next iteration
                    }
                    yield();
                }
                assetFile = root.openNextFile();
            }
        }
        output += "}}";

        request->send(200, "application/json", output);
        Serial.printf("BACKUP: Response size = %u bytes\n", output.length());
    });

    static const size_t MAX_RESTORE_SIZE = 512 * 1024;

    server.on("/api/restore", HTTP_POST, [](AsyncWebServerRequest* request) {
        Serial.println("RESTORE: request handler called");
        set_pending_redundant_write(true);

        // Retrieve the accumulated body from tempObject
        char* body = (char*)request->_tempObject;
        if (!body || body[0] == '\0') {
            Serial.println("RESTORE: FAILED - no body data accumulated");
            if (body) { free(body); request->_tempObject = NULL; }
            set_pending_redundant_write(false);
            request->send(400, "text/plain", "No body data");
            return;
        }

        size_t body_len = strlen(body);
        Serial.printf("RESTORE: Processing body, size=%u\n", body_len);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            Serial.printf("RESTORE: FAILED - JSON parse error: %s\n", error.c_str());
            free(body);
            request->_tempObject = NULL;
            set_pending_redundant_write(false);
            request->send(400, "text/plain", "JSON Parse Error");
            return;
        }
        Serial.println("RESTORE: JSON parsed OK");

        if (!doc["bg"].isNull()) { g_bg_color = parse_color(doc["bg"]); Serial.printf("RESTORE: bg=#%06X\n", g_bg_color); }
        if (!doc["rows"].isNull()) { g_rows = doc["rows"]; if (g_rows < 1 || g_rows > 5) g_rows = 3; Serial.printf("RESTORE: rows=%u\n", g_rows); }
        if (!doc["cols"].isNull()) { g_cols = doc["cols"]; if (g_cols < 1 || g_cols > 5) g_cols = 3; Serial.printf("RESTORE: cols=%u\n", g_cols); }
        if (!doc["os"].isNull()) { g_target_os = doc["os"]; Serial.printf("RESTORE: os=%u (%s)\n", g_target_os, g_target_os == OS_WINDOWS ? "Windows" : "macOS"); }
        if (!doc["lang"].isNull()) { g_kb_lang = doc["lang"]; Serial.printf("RESTORE: lang=%u\n", g_kb_lang); }
        if (!doc["wifi_ssid"].isNull()) { strncpy(g_wifi_ssid, doc["wifi_ssid"], 31); Serial.printf("RESTORE: wifi_ssid=%s\n", g_wifi_ssid); }

        auto restore_btns = [&](JsonArray arr, const char* path) {
            Serial.printf("RESTORE: Writing %s with %u buttons\n", path, arr.size());
            ButtonConfig btns[MAX_BUTTONS];
            memset(btns, 0, sizeof(btns));
            for (int i = 0; i < MAX_BUTTONS && i < arr.size(); i++) {
                JsonObject b = arr[i];
                strncpy(btns[i].label, b["label"] | "Button", 15);
                strncpy(btns[i].value, b["value"] | "", 255);
                btns[i].type = b["type"] | 0;
                btns[i].color = parse_color(b["color"] | "333333");

                String iconVal = b["icon"] | "";
                if (iconVal == "None" || iconVal.isEmpty()) {
                    btns[i].icon[0] = '\0';
                } else {
                    String rawIcon = iconVal;
                    iconVal.toLowerCase();
                    bool found = false;
                    for (int j = 0; j < 20; j++) {
                        String name(g_sym_names[j]);
                        name.toLowerCase();
                        if (iconVal == name) {
                            const char* sym = g_sym_codes[j];
                            size_t sym_len = strlen(sym);
                            if (sym_len > 0 && sym_len < 8) {
                                strncpy(btns[i].icon, sym, sym_len);
                                btns[i].icon[sym_len] = '\0';
                            } else {
                                btns[i].icon[0] = '\0';
                            }
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        strncpy(btns[i].icon, rawIcon.c_str(), 7);
                        btns[i].icon[7] = '\0';
                    }
                }

                strncpy(btns[i].imgPath, b["img"] | "", 31);
            }
            File f = LittleFS.open(path, "w");
            if (f) {
                f.write((uint8_t*)btns, sizeof(btns));
                f.close();
                Serial.printf("RESTORE: %s written OK (%u bytes)\n", path, sizeof(btns));
            } else {
                Serial.printf("RESTORE: FAILED to open %s for writing\n", path);
            }
        };

        if (!doc["win_btns"].isNull()) {
            Serial.printf("RESTORE: win_btns array has %u entries\n", doc["win_btns"].as<JsonArray>().size());
            restore_btns(doc["win_btns"].as<JsonArray>(), "/win_btns.bin");
        }
        if (!doc["mac_btns"].isNull()) {
            Serial.printf("RESTORE: mac_btns array has %u entries\n", doc["mac_btns"].as<JsonArray>().size());
            restore_btns(doc["mac_btns"].as<JsonArray>(), "/mac_btns.bin");
        }

        // Optimized Base64 decode with pre-allocated buffer + yield points
        static auto decode_b64 = [](const String& input, std::vector<uint8_t>& output) -> bool {
            size_t approx = (input.length() / 4) * 3 + 3;
            output.resize(approx);
            size_t outPos = 0;
            int i = 0;
            uint8_t char_array_4[4], char_array_3[3];
            auto b64_char = [](char c) -> int {
                if (c >= 'A' && c <= 'Z') return c - 'A';
                if (c >= 'a' && c <= 'z') return c - 'a' + 26;
                if (c >= '0' && c <= '9') return c - '0' + 52;
                if (c == '+') return 62;
                if (c == '/') return 63;
                return -1;
            };
            for (size_t in_ = 0; in_ < input.length(); in_++) {
                char c = input[in_];
                if (c == '=') break;
                int val = b64_char(c);
                if (val == -1) continue;
                char_array_4[i++] = (uint8_t)val;
                if (i == 4) {
                    if (outPos + 3 > output.size()) output.resize(output.size() + 3);
                    char_array_3[0] = (char_array_4[0] << 2) | ((char_array_4[1] & 0x30) >> 4);
                    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) | ((char_array_4[2] & 0x3c) >> 2);
                    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) | char_array_4[3];
                    output[outPos++] = char_array_3[0];
                    output[outPos++] = char_array_3[1];
                    output[outPos++] = char_array_3[2];
                    i = 0;
                }
            }
            if (i) {
                for (int j = i; j < 4; j++) char_array_4[j] = 0;
                if (outPos + 3 > output.size()) output.resize(output.size() + 3);
                char_array_3[0] = (char_array_4[0] << 2) | ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) | ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) | char_array_4[3];
                for (int j = 0; j < i - 1; j++) output[outPos++] = char_array_3[j];
            }
            output.resize(outPos);
            return outPos > 0;
        };

        if (!doc["assets"].isNull()) {
            JsonObject assets = doc["assets"].as<JsonObject>();
            int assetCount = 0;
            int assetTotal = 0;
            for (JsonPair kv : assets) { assetTotal++; }

            std::vector<uint8_t> decodeBuf;
            decodeBuf.reserve(65536);  // pre-allocate 64KB for decode buffer

            for (JsonPair kv : assets) {
                String filename = kv.key().c_str();
                if (!filename.startsWith("/")) filename = "/" + filename;
                String b64 = kv.value().as<String>();

                decodeBuf.clear();
                if (decode_b64(b64, decodeBuf) && decodeBuf.size() > 0) {
                    File f = LittleFS.open(filename, "w");
                    if (f) {
                        f.write(decodeBuf.data(), decodeBuf.size());
                        f.close();
                    }
                }
                assetCount++;
                Serial.printf("RESTORE: Asset[%d/%d] %s (%u bytes)\n", assetCount, assetTotal, filename.c_str(), decodeBuf.size());

                // Yield every 5 assets to let watchdog and UI breathe
                if (assetCount % 5 == 0) {
                    io_yield();
                }
            }
            Serial.printf("RESTORE: Assets done (%d/%d)\n", assetCount, assetTotal);
        } else {
            Serial.println("RESTORE: No assets in backup");
        }

        {
            const char* restore_file = (g_target_os == OS_WINDOWS ? "/win_btns.bin" : "/mac_btns.bin");
            File ff = LittleFS.open(restore_file, "r");
            if (ff) {
                ff.read((uint8_t*)g_configs, sizeof(g_configs));
                ff.close();
            }
        }

        save_settings(true);
        set_pending_redundant_write(false);

        mark_all_dirty();
        g_pending_ui_update = true;

        free(body);
        request->_tempObject = NULL;

        request->send(200, "text/plain", "Restore OK");
        Serial.println("RESTORE: Done");
    }, NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        // Body handler: accumulate raw body data into request->_tempObject
        if (index == 0) {
            if (request->_tempObject) {
                free(request->_tempObject);
                request->_tempObject = NULL;
            }
        }
        if (total == 0 || total > MAX_RESTORE_SIZE) {
            return;
        }
        if (!request->_tempObject) {
            request->_tempObject = malloc(total + 1);
            if (!request->_tempObject) {
                return;
            }
            memset(request->_tempObject, 0, total + 1);
        }
        memcpy((uint8_t*)(request->_tempObject) + index, data, len);
    });

    server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        File root = LittleFS.open("/");
        if (root) {
            File file = root.openNextFile();
            while (file) {
                String name = String(file.name());
                if (name != "win_btns.bin" && name != "mac_btns.bin") {
                    JsonObject f = arr.add<JsonObject>();
                    f["name"] = name;
                    f["size"] = file.size();
                }
                file = root.openNextFile();
            }
        }
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/api/delete", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("filename", true)) {
            String fname = request->getParam("filename", true)->value();
            if (!fname.startsWith("/")) fname = "/" + fname;

            if (fname == "/win_btns.bin" || fname == "/mac_btns.bin") {
                request->send(403, "text/plain", "Forbidden: System File");
                return;
            }

            if (LittleFS.remove(fname)) {
                request->send(200, "text/plain", "OK");
            } else {
                request->send(500, "text/plain", "Delete failed");
            }
        } else {
            request->send(400, "text/plain", "Missing filename");
        }
    });

    // OTA Update handler
    static bool g_ota_success = false;
    static bool g_ota_failed = false;
    static size_t g_ota_total_size = 0;
    static size_t g_ota_content_length = 0;
    static String g_ota_error_msg = "";

    server.on("/api/update", HTTP_POST, [](AsyncWebServerRequest* request) {
        bool shouldRestart = g_ota_success && !Update.hasError() && !g_ota_failed;

        String response_msg;
        int response_code = 200;

        if (shouldRestart) {
            response_msg = "Update OK. Restarting...";
        } else if (g_ota_failed) {
            response_msg = "Update Failed: " + g_ota_error_msg;
            response_code = 500;
        } else if (Update.hasError()) {
            response_msg = "Update Error";
            response_code = 500;
        } else {
            response_msg = "Update incomplete";
            response_code = 500;
        }

        AsyncWebServerResponse* response = request->beginResponse(response_code, "text/plain", response_msg);
        response->addHeader("Connection", "close");
        request->send(response);

        if (shouldRestart) {
            Serial.println("OTA: SUCCESS - Sending restart command");
            g_ota_progress = -1;
            delay(1000);
            Serial.flush();
            delay(500);
            ESP.restart();
        } else {
            g_ota_success = false;
            g_ota_failed = false;
            g_ota_progress = -1;
        }
    }, [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
        if (!index) {
            g_ota_success = false;
            g_ota_failed = false;
            g_ota_total_size = 0;
            g_ota_content_length = 0;
            g_ota_error_msg = "";

            g_ota_content_length = request->contentLength();
            Serial.println("OTA: Starting firmware update via web interface...");
            Serial.printf("OTA: Request Content-Length: %zu bytes\n", g_ota_content_length);

            if (g_ota_content_length > 4 * 1024 * 1024) {
                g_ota_error_msg = String("Invalid size: ") + String((float)g_ota_content_length / (1024.0 * 1024.0), 2) + "MB";
                g_ota_failed = true;
                Serial.printf("OTA: ERROR - Content too large: %zu bytes\n", g_ota_content_length);
                delay(2000);
                return;
            }

            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                g_ota_error_msg = "Cannot begin update";
                g_ota_failed = true;
                Serial.print("OTA: Update.begin() failed: ");
                Update.printError(Serial);
                delay(2000);
                return;
            }

            g_ota_screen_requested = true;
            g_ota_progress = 0;
            Serial.println("OTA: Update.begin() successful (size unknown - multipart)");
        }

        if (len) {
            if (g_ota_failed) {
                Serial.println("OTA: Skipping write, already failed");
                return;
            }

            size_t written = Update.write(data, len);
            g_ota_total_size += written;

            if (g_ota_content_length > 0) {
                g_ota_progress = (g_ota_total_size * 100) / g_ota_content_length;
                if (g_ota_progress > 99) g_ota_progress = 99;
            }

            if (written != len) {
                g_ota_error_msg = "Write error";
                g_ota_failed = true;
                Serial.printf("OTA: Write mismatch - Expected: %u, Written: %u\n", len, written);
                Update.printError(Serial);
                delay(2000);
                return;
            }

            // Use esp_task_wdt_reset instead of deinit to keep watchdog active
            for (int i = 0; i < 50; i++) {
                esp_task_wdt_reset();
                yield();
                delayMicroseconds(100);
            }
        }

        if (final) {
            if (g_ota_failed) return;

            Serial.printf("OTA: Final chunk - Total received: %u bytes\n", g_ota_total_size);

            for (int i = 0; i < 100; i++) {
                esp_task_wdt_reset();
                yield();
                delayMicroseconds(2000);
            }
            delay(2000);

            if (!Update.end(true)) {
                g_ota_error_msg = "Update end failed";
                g_ota_failed = true;
                Serial.print("OTA: Update.end() failed: ");
                Update.printError(Serial);
                delay(2000);
                return;
            }

            if (Update.hasError()) {
                g_ota_error_msg = "Update error after end";
                g_ota_failed = true;
                Serial.print("OTA: Update has error after end: ");
                Update.printError(Serial);
                delay(2000);
                return;
            }

            g_ota_progress = 100;
            Serial.println("OTA: Update successful! Restarting...");
            g_ota_success = true;
        }
    });

    server.on("/api/upload", HTTP_POST, [](AsyncWebServerRequest* request) {
        Serial.println("API: Upload complete.");
        request->send(200, "text/plain", "Upload OK");
    }, [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
        static File uploadFile;
        if (!index) {
            if (!filename.startsWith("/")) filename = "/" + filename;
            Serial.printf("API: Receiving file %s\n", filename.c_str());
            uploadFile = LittleFS.open(filename, "w");
        }
        if (len && uploadFile) uploadFile.write(data, len);
        if (final && uploadFile) {
            uploadFile.close();
            Serial.println("API: File saved to LittleFS.");
        }
    });

    // Main dashboard page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Replace version placeholder
        String html = FPSTR(INDEX_HTML);
        html.replace("__VERSION__", PANDA_VERSION);

        AsyncWebServerResponse* response = request->beginResponse(200, "text/html; charset=utf-8", html);
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        request->send(response);
    });

    server.begin();
    webserver_started = true;
    Serial.println("Web Server started.");
}
