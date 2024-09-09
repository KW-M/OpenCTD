// Include guard
#ifndef CONFIG_STORAGE_H
#define CONFIG_STORAGE_H

#include "Arduino.h"
#include <MultiMap.h>
#undef nullptr
#include <SPI.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>

#include "ssrov_ctd_pinouts_and_constants.hpp"
// #include "command_mode.hpp"

// this file requires these libries from arduino library manager
// - ArduinoSTL by Mike Matera (used version 1.3.3)
// - ArduinoStreamUtils by Benoit Blanchon
// - ArduinoJson by Benoit Blanchon (used version 6.19.3)
// - MultiMap by Rob Tillaart

typedef struct
{
  float *measuredValues; // list of measured values from the sensor
  float *realValues;     // list of real values that correspond to the measured values the sensor spits out.
  int length = 0;
} SensorCalibration_t;

extern onboard_config_type default_onboard_config;
extern onboard_config_type onboard_config;

/****************** Forward Function Header Definitions **********************
  allows us to use these functions anywhere past this point, even though
  they are actually filled out in files that are included later
*/

char* temp_get_highest_probe_address();

// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

extern char feather_board_serial_number[33];
extern char highest_temp_probe_address_str[17];

// -------------------------------------
// -------------- Functions -----------
// -------------------------------------

#if defined(ARDUINO_ARCH_SAMD) // ------------- < This means we are using a feather m0 (or a board with the same processor)
char *
get_feather_serial_number();
#endif // ----------------------------------------------------------------------------------------

bool json_config_to_struct(JsonDocument &doc);
void onboard_struct_to_json_config(JsonDocument &doc);
String get_onboard_config_file_path();
String get_tube_config_file_path();
void write_onboard_config();
void read_onboard_config();
void setup_config_storage();
void write_sensor_calibration(String jsonKey, bool onboardComponent, SensorCalibration_t *sensorCalibration);
SensorCalibration_t read_sensor_calibration(String jsonKey, bool onboardComponent);
void clear_sensor_calibration(SensorCalibration_t *sensorCalibration);
void add_calibration_point(float measuredValue, float realValue, SensorCalibration_t *sensorCalibration);
float calculate_calibrated_value(float measuredValue, SensorCalibration_t *sensorCalibration);
void print_calibration_values(SensorCalibration_t *sensorCalibration);

#endif
