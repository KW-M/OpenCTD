// Double include guard

#ifndef PRESSURE_SENSOR_FUNCTIONS_H
#define PRESSURE_SENSOR_FUNCTIONS_H
#include "ssrov_ctd_pinouts_and_constants.hpp"
#if ENABLE_PRESSURE_SENSOR

#include "command_mode.hpp"

#if !defined(PRESSURE_SENSOR_RESOLUTION)
#error "Make sure your pinnouts & constants.h is included first and contains the constants listed in the !defined statements above this error in the source code."
#endif

// Software libraries for the pressure sensor
#include <Wire.h>
// #include <MS5803_14.h>
#include <SparkFun_MS5803_I2C.h> // Click here to get the library: http://librarymanager/All#SparkFun_MS5803-14BA


// ----------------------------------------
// -------- Presure Sensor Functions --------
// ----------------------------------------

void pressure_setup_sensor();
float pressure_get_raw_reading();
float pressure_get_calibrated_reading();
float pressure_get_value();

bool pressure_sensor_user_command_handler(UserCommand,const __FlashStringHelper *);
void pressure_sensor_show_live_data();

#endif
#endif
