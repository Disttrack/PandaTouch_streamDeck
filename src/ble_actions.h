#pragma once
#ifndef BLE_ACTIONS_H
#define BLE_ACTIONS_H

#include <Arduino.h>
#include "constants.h"

void init_ble();
void ble_write(char c);
void execute_adv_shortcut(const char* value);
void handle_button_action(uint8_t idx);
bool is_ble_connected();
void check_ble_status();

#endif
