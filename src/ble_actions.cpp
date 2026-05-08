#include "ble_actions.h"
#include "storage.h"
#include <BleKeyboard.h>
#include <BLEDevice.h>
#include <BLESecurity.h>

BleKeyboard bleKeyboard("PandaTouch Deck", "BigTreeTech", 100);

void init_ble() {
    Serial.println("Setting BLE security to NONE...");
    BLESecurity* pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_NO_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    Serial.println("Calling bleKeyboard.begin()...");
    bleKeyboard.begin();
    Serial.println("bleKeyboard.begin() completed");
    Serial.println("BLE Keyboard initialized. Device name: PandaTouch Deck");
    Serial.println("Waiting for Bluetooth connection...");
}

bool is_ble_connected() {
    return bleKeyboard.isConnected();
}

void check_ble_status() {
    static bool was_connected = false;
    static unsigned long last_check = 0;
    bool is_connected = bleKeyboard.isConnected();

    if (is_connected != was_connected) {
        if (is_connected) {
            Serial.println("*** BLE CONNECTED! ***");
        } else {
            Serial.println("*** BLE DISCONNECTED ***");
        }
        was_connected = is_connected;
    }

    if (!is_connected && (millis() - last_check > 10000)) {
        Serial.println("Still advertising... Waiting for connection.");
        last_check = millis();
    }
}

void ble_write(char c) {
    if (!bleKeyboard.isConnected()) return;

    if (g_kb_lang == LANG_US) {
        bleKeyboard.write(c);
    } else if (g_kb_lang == LANG_ES) {
        switch (c) {
            case '"': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('2'); bleKeyboard.releaseAll(); break;
            case '=': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('0'); bleKeyboard.releaseAll(); break;
            case '(': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('8'); bleKeyboard.releaseAll(); break;
            case ')': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('9'); bleKeyboard.releaseAll(); break;
            case '&': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('6'); bleKeyboard.releaseAll(); break;
            case ':': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('.'); bleKeyboard.releaseAll(); break;
            case ';': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write(','); bleKeyboard.releaseAll(); break;
            case '/': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('7'); bleKeyboard.releaseAll(); break;
            case '?': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('\''); bleKeyboard.releaseAll(); break;
            case '\\': bleKeyboard.press(KEY_LEFT_CTRL); bleKeyboard.press(KEY_LEFT_ALT); bleKeyboard.write('`'); bleKeyboard.releaseAll(); break;
            case '+': bleKeyboard.write('['); break;
            case '*': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('['); bleKeyboard.releaseAll(); break;
            case '-': bleKeyboard.write('/'); break;
            case '_': bleKeyboard.press(KEY_LEFT_SHIFT); bleKeyboard.write('/'); bleKeyboard.releaseAll(); break;
            default: bleKeyboard.write(c); break;
        }
    }
}

void execute_adv_shortcut(const char* value) {
    if (!value || value[0] == '\0') return;

    String val = String(value);
    val.toUpperCase();

    int lastPos = 0;
    while (lastPos < val.length()) {
        int plusPos = val.indexOf('+', lastPos);
        String part = (plusPos == -1) ? val.substring(lastPos) : val.substring(lastPos, plusPos);
        part.trim();

        if (part == "CTRL") bleKeyboard.press(KEY_LEFT_CTRL);
        else if (part == "SHIFT") bleKeyboard.press(KEY_LEFT_SHIFT);
        else if (part == "ALT") bleKeyboard.press(KEY_LEFT_ALT);
        else if (part == "GUI" || part == "WIN" || part == "CMD") bleKeyboard.press(KEY_LEFT_GUI);
        else if (part == "ENTER" || part == "RETURN") bleKeyboard.press(KEY_RETURN);
        else if (part == "TAB") bleKeyboard.press(KEY_TAB);
        else if (part == "ESC") bleKeyboard.press(KEY_ESC);
        else if (part == "BACKSPACE") bleKeyboard.press(KEY_BACKSPACE);
        else if (part == "DEL" || part == "DELETE") bleKeyboard.press(KEY_DELETE);
        else if (part == "UP") bleKeyboard.press(KEY_UP_ARROW);
        else if (part == "DOWN") bleKeyboard.press(KEY_DOWN_ARROW);
        else if (part == "LEFT") bleKeyboard.press(KEY_LEFT_ARROW);
        else if (part == "RIGHT") bleKeyboard.press(KEY_RIGHT_ARROW);
        else if (part == "SPACE") bleKeyboard.press(' ');
        else if (part.startsWith("F") && part.length() > 1) {
            int fNum = part.substring(1).toInt();
            switch (fNum) {
                case 1: bleKeyboard.press(KEY_F1); break;
                case 2: bleKeyboard.press(KEY_F2); break;
                case 3: bleKeyboard.press(KEY_F3); break;
                case 4: bleKeyboard.press(KEY_F4); break;
                case 5: bleKeyboard.press(KEY_F5); break;
                case 6: bleKeyboard.press(KEY_F6); break;
                case 7: bleKeyboard.press(KEY_F7); break;
                case 8: bleKeyboard.press(KEY_F8); break;
                case 9: bleKeyboard.press(KEY_F9); break;
                case 10: bleKeyboard.press(KEY_F10); break;
                case 11: bleKeyboard.press(KEY_F11); break;
                case 12: bleKeyboard.press(KEY_F12); break;
            }
        } else if (part.length() == 1) {
            char c = part[0];
            if (c >= 'A' && c <= 'Z') c += 32;
            bleKeyboard.press(c);
        }

        if (plusPos == -1) break;
        lastPos = plusPos + 1;
    }

    delay(100);
    bleKeyboard.releaseAll();
}

void handle_button_action(uint8_t idx) {
    if (!bleKeyboard.isConnected()) {
        Serial.println("BLE not connected!");
        return;
    }

    if (idx >= MAX_BUTTONS) return;
    ButtonConfig& cfg = g_configs[idx];

    if (cfg.type == BTN_TYPE_APP) {
        if (g_target_os == OS_WINDOWS) {
            bleKeyboard.press(KEY_LEFT_GUI);
            bleKeyboard.press('r');
            delay(150);
            bleKeyboard.releaseAll();
            delay(500);
        } else {
            bleKeyboard.press(KEY_LEFT_GUI);
            bleKeyboard.press(' ');
            delay(150);
            bleKeyboard.releaseAll();
            delay(300);
        }

        for (int i = 0; cfg.value[i]; i++) {
            ble_write(cfg.value[i]);
            delay(5);
        }

        delay(200);
        bleKeyboard.write(KEY_RETURN);
    } else if (cfg.type == BTN_TYPE_MEDIA) {
        if (strcmp(cfg.value, "mute") == 0) bleKeyboard.write(KEY_MEDIA_MUTE);
        else if (strcmp(cfg.value, "volup") == 0) bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        else if (strcmp(cfg.value, "voldown") == 0) bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        else if (strcmp(cfg.value, "play") == 0) bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
        else if (strcmp(cfg.value, "next") == 0) bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
        else if (strcmp(cfg.value, "prev") == 0) bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
        else if (strcmp(cfg.value, "stop") == 0) bleKeyboard.write(KEY_MEDIA_STOP);
    } else if (cfg.type == BTN_TYPE_BASIC_COMBO) {
        if (g_target_os == OS_WINDOWS) bleKeyboard.press(KEY_LEFT_CTRL);
        else bleKeyboard.press(KEY_LEFT_GUI);

        bleKeyboard.press(cfg.value[0]);
        delay(100);
        bleKeyboard.releaseAll();
    } else if (cfg.type == BTN_TYPE_ADV_COMBO) {
        execute_adv_shortcut(cfg.value);
    }
}
