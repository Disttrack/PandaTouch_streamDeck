#include "streamdeck.h"
#include "storage.h"
#include "ui_main.h"
#include "ble_actions.h"
#include "webserver.h"
#include "l10n.h"
#include <ArduinoOTA.h>

void StreamDeckApp::setup() {
    WiFi.mode(WIFI_STA);

    init_storage();
    load_settings();

    g_main_screen = lv_scr_act();
    create_main_ui();

    Serial.println("StreamDeckApp::setup() - Starting BLE initialization");
    delay(500);

    init_ble();

    ArduinoOTA.onStart([]() {
        show_update_screen();
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("OTA: Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        update_ota_progress(100, "Update Complete!");
        Serial.println("\nOTA: Update Complete");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        int pct = (progress / (total / 100));
        update_ota_progress(pct, "Updating System...");
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
    check_ble_status();
    check_wifi_status();

    if (g_pending_ui_update) {
        g_pending_ui_update = false;
        lv_scr_load(g_main_screen);
        create_main_ui();
    }

    ArduinoOTA.handle();

    delay(1);
}

void StreamDeckApp::handle_button(uint8_t idx) {
    handle_button_action(idx);
}

void StreamDeckApp::log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
