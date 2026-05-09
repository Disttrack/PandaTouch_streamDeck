#include "pt/pt_display.h"
#include "streamdeck.h"
#include "storage.h"
#include "ui_main.h"
#include "esp32-hal-cpu.h"

void setup()
{
  setCpuFrequencyMhz(240);

  pinMode(21, OUTPUT); digitalWrite(21, 0);
  delay(500);

  Serial.begin(115200);
  delay(200);
  Serial.println("\n\n=== PandaTouch StreamDeck Starting ===");

  pt_setup_display(PT_LVGL_RENDER_FULL_2);
  StreamDeckApp::setup();
  pt_set_backlight(g_brightness, true);
}

void set_brightness(uint8_t val) {
  pt_set_backlight(val, true);
}

void loop()
{
  pt_loop_display();
  StreamDeckApp::loop();
}
