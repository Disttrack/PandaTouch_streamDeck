#pragma once
#ifndef STREAMDECK_H
#define STREAMDECK_H

#include <Arduino.h>
#include "constants.h"

class StreamDeckApp {
public:
    static void setup();
    static void loop();
    static void handle_button(uint8_t action_id);
    static void log(const char* fmt, ...);
};

#endif
