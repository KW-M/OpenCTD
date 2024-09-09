// Double include guard
#ifndef LIGHT_SENSOR_FUNCTIONS_H
#define LIGHT_SENSOR_FUNCTIONS_H
#include "ssrov_ctd_pinouts_and_constants.hpp"
#if ENABLE_LIGHT_SENSOR


#include "command_mode.hpp"
#include <Arduino.h>
#include <hp_BH1750.h>  //  include the library

#if !defined(LIGHT_SENSOR_I2C_ADDR)
#error "Make sure your pinnouts & constants.hpp is included first and contains the constants listed in the !defined statements above this error in the source code."
#endif

// ----------------------------------------
// -------- Light Sensor Functions --------
// ----------------------------------------

bool light_sensor_setup_sensor();
float light_sensor_get_raw_light_level();
bool light_sensor_is_calibrated();
float light_sensor_get_lux_light_level();
float light_sensor_get_value();
bool light_sensor_refresh_value();
bool light_sensor_user_command_handler(UserCommand, const __FlashStringHelper *);
void light_sensor_show_live_data();

#endif

#endif
