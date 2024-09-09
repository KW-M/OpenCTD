// Double include guard
#ifndef POWER_CTRL_H
#define POWER_CTRL_H

#include "Arduino.h"

extern volatile bool board_will_power_down;

void power_ctrl_setup();
float power_ctrl_get_switch_voltage();
void power_ctrl_check_switch();

#endif
