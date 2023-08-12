// Double include guard
#ifndef PHOTORESISTOR_FUNCTIONS_H
#define PHOTORESISTOR_FUNCTIONS_H
#include "ssrov_ctd_pinouts_and_constants.hpp"
#if ENABLE_PHOTORESISTOR

#include "utility_functions.hpp"
#include "power_control.hpp"
#include "command_mode.hpp"
#include "config_storage.hpp"
#include "sdcard.hpp"

#if !defined(PHOTORESISTOR_ENABLE_PIN) or !defined(PHOTORESISTOR_SENSE_PIN) or !defined(PHOTORESISTOR_SAMPLE_AVERAGING_COUNT)
#error "Make sure your pinnouts & constants.hpp is included first and contains the constants listed in the !defined statements above this error in the source code."
#endif

// ----------------------------------------
// -------- Photoresitor Functions --------
// ----------------------------------------

void photoresistor_setup_sensor();
float photoresistor_get_raw_light_level();
bool photoresistor_is_calibrated();
float photoresistor_get_lux_light_level();
void photoresistor_log_value();
void photoresistor_command_mode_loop();

#endif

#endif
