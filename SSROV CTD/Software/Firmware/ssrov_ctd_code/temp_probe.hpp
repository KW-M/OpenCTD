// Double include guard
#ifndef TEMP_PROBE_FUNCTIONS_H
#define TEMP_PROBE_FUNCTIONS_H
#include "ssrov_ctd_pinouts_and_constants.hpp"
#if ENABLE_TEMP_PROBE

#include "Arduino.h"

// include the communication protocol libs for temperature sensors
// these two are used by the library to exclude
// code for certain temp probe features,
// but they must go before the temp probe library is included.
#define REQUIRESNEW false
#define REQUIRESALARMS false // include the communication protocol libs for temperature sensors (must go here so the DeviceAddress types are defined earlier)
#include <OneWire.h>
#include <DallasTemperature.h>
#include "command_mode.hpp"



// ----------------------------------------
// -------- Functions --------
// ----------------------------------------

void temp_setup_sensors();
void temp_probes_refresh_values();
float temp_get_latest_raw_value_by_probe_index(int probe_indx);
float temp_get_latest_calibrated_value_by_probe_index(int probe_indx);
float temp_get_value(uint8_t probe_indx);
bool temp_get_probe_address(DeviceAddress *temp_probe_address_destination, uint8_t probe_indx);
bool temp_get_probe_address_str(char *temp_probe_address_str_destination, uint8_t probe_indx);
char *temp_get_highest_probe_address();


void temp_sensor_show_live_data();
bool temp_sensor_user_command_handler(UserCommand,const __FlashStringHelper *);


#endif
#endif
