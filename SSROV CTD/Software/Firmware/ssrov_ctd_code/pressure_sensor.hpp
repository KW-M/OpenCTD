// Double include guard

#ifndef PRESSURE_SENSOR_FUNCTIONS_H
#define PRESSURE_SENSOR_FUNCTIONS_H
#include "ssrov_ctd_pinouts_and_constants.hpp"
#if ENABLE_PRESSURE_SENSOR

#include "utility_functions.hpp"
#include "power_control.hpp"
#include "command_mode.hpp"
#include "config_storage.hpp"
#include "sdcard.hpp"

#if !defined(PRESSURE_SENSOR_RESOLUTION)
#error "Make sure your pinnouts & constants.h is included first and contains the constants listed in the !defined statements above this error in the source code."
#endif

// Software libraries for the pressure sensor
#include <Wire.h>
#include <MS5803_14.h>

// ----------------------------------------
// -------- Presure Sensor Functions --------
// ----------------------------------------

void pressure_setup_sensor();
float pressure_get_raw_reading();
float pressure_get_calibrated_reading();
void pressure_log_value();
void pressure_command_mode_loop();

#endif
#endif
