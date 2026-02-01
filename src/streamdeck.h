#ifndef STREAMDECK_H
#define STREAMDECK_H

#include <Arduino.h>
#include <stdint.h>
#include "lvgl.h" 

#define PANDA_VERSION "1.5.4"

class StreamDeckApp {
public:
    static void setup();
    static void loop();
    static void handle_button(uint8_t action_id); 
    static void log(const char* fmt, ...);
};

#endif // STREAMDECK_H
