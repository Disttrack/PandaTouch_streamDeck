#include "pt/pt_display.h"
#include "streamdeck.h"

void setup()
{
  // Safety delay to allow USB to enumerate and Serial to connect
  pinMode(21, OUTPUT); digitalWrite(21, 0); 
  delay(3000);
  
  // Enable Serial for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== PandaTouch StreamDeck " PANDA_VERSION " Starting ===");
  
  pt_setup_display(PT_LVGL_RENDER_FULL_1);
  pt_set_backlight(50, true);
  StreamDeckApp::setup();
}

void set_brightness(uint8_t val) {
  pt_set_backlight(val, true);
}

void loop()
{
  pt_loop_display();
  StreamDeckApp::loop();
}