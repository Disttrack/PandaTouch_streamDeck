#pragma once
#ifndef PT_CONSTANTS_H
#define PT_CONSTANTS_H

#include <stdint.h>

#define MAX_BUTTONS 20
#define PANDA_VERSION "1.6.1"

enum ButtonType {
    BTN_TYPE_APP = 0,
    BTN_TYPE_MEDIA,
    BTN_TYPE_BASIC_COMBO,
    BTN_TYPE_ADV_COMBO
};

enum TargetOS {
    OS_WINDOWS = 0,
    OS_MACOS
};

enum KbLang {
    LANG_US = 0,
    LANG_ES
};

#endif
