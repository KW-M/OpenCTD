// Double include guard
#ifndef CALIBRATION_STORAGE_FUNCTIONS_H
#define CALIBRATION_STORAGE_FUNCTIONS_H

#include "ssrov_ctd_pinouts_and_constants.hpp"
#include "utility_functions.hpp"
// #include "command_mode_functions.h"
// --------------------------------------------
// #include "clock.hpp"
// #include "sdcard.hpp"
// #include "battery_functions.h"
// #include "indicator_light.hpp"
// #include "conductivity_sensor_functions.h"
// #include "photoresistor_functions.h"
// #include "pressure_sensor_functions.h"
// #include "temp_probe_functions.h"
// #include "calibration_storage_functions.h"

#ifdef NOTDEFINED_ON_PERPOUSE
bool save_configs_to_sd_card()
{

  // Make sure the folder for storing the saved configurations actually exists first:
  if (not sd.exists(CONFIG_STORAGE_FOLDER_PATH) && !sd.mkdir(CONFIG_STORAGE_FOLDER_PATH, true))
  {
    Serial.print(F("Failed to create config storage directory: "));
    Serial.println(CONFIG_STORAGE_FOLDER_PATH);
    return false;
  }

  // temporary variables used in the rest of the function
  bool saving_was_successful = true; // this will get set to false if any errors occur and returned at the end
  char configFilePath[100];
  File32 saved_calibration_file;

  // ----- Create/Write config file for components attached to the feather board itself: ----

  if (strlen(feather_board_serial_number) >= 16)
  { //  make sure the feather_board_serial_number was fully read (not less than 16 charaters) before creating file

    sprintf(configFilePath, "%sonboard_config-v%i-%s.bin", CONFIG_STORAGE_FOLDER_PATH, CONFIG_VERSION_NUMBER, feather_board_serial_number);
    saved_calibration_file.open(configFilePath, O_WRONLY | O_CREAT); // FILE_WRITE would append to the end of the file instead of what we want (overwrite).

    if (saved_calibration_file)
    {
      Serial.println("Saving onboard calibration to sd card...");
      saved_calibration_file.write((uint8_t *)&onboard_config, sizeof(onboard_config) / sizeof(uint8_t)); // write the whole onboard_config struct from the bytes in arduino memory onto the sd card.
      saved_calibration_file.close();                                                                     // close the file:
      // EEPROM_writeAnything(0, onboard_config);
    }
    else
    {
      print_sd_card_error(configFilePath); // if the file didn't open, print an error with the file path.
      saving_was_successful = false;       // signal that the loading was NOT successful
    }
  }
  else
  {
    Serial.println(F("Could not save onboard components calibration file, because the serial number of the feather board could not be determined."));
    saving_was_successful = false;
  }

  // ----- Create/Write config file for components attached to the ctd tube: -----
  if (strlen(highest_temp_probe_address_str) >= 16)
  { //  make sure the probe address was fully read (not less than 16 charaters) before creating file

    sprintf(configFilePath, "%stube_components_config-v%i-%s.bin", CONFIG_STORAGE_FOLDER_PATH, CONFIG_VERSION_NUMBER, highest_temp_probe_address_str);
    saved_calibration_file.open(configFilePath, O_WRONLY | O_CREAT); // FILE_WRITE would append to the end of the file instead of what we want (overwrite).

    if (saved_calibration_file)
    {
      Serial.println("Saving tube calibration to sd card...");
      saved_calibration_file.write((uint8_t *)&tube_components_config, sizeof(tube_components_config) / sizeof(uint8_t)); // write the whole tube_components_config struct from the bytes in arduino memory onto the sd card.
      saved_calibration_file.close();                                                                                     // close the file:
      // EEPROM_writeAnything(0, tube_components_config);
    }
    else
    {
      print_sd_card_error(configFilePath); // if the file didn't open, print an error with the file path.
      saving_was_successful = false;
    }
  }
  else
  {
    Serial.println(F("Could not save tube components calibration file, because the addresses of the temperature propes in the tube could not be determined."));
    saving_was_successful = false;
  }

  // Return the saving_was_successful variable to signal if the loading was successful (no errors happened above)
  return saving_was_successful;
}

bool load_configs_from_sd_card()
{

  // temporary variables used in the rest of the function
  bool loading_was_successful = true; // this will get set to false if any errors occur and returned at the end
  char configFilePath[100];
  File32 saved_calibration_file;

  // ----- load config file for components attached to the feather board itself: ----
  if (strlen(feather_board_serial_number) > 0)
  {
    sprintf(configFilePath, "%sonboard_config-v%i-%s.bin", CONFIG_STORAGE_FOLDER_PATH, CONFIG_VERSION_NUMBER, feather_board_serial_number);
    if (sd.exists(configFilePath))
    {
      saved_calibration_file.open(configFilePath, FILE_READ);
      if (saved_calibration_file)
      {
        Serial.print(F("Reading feather calibration file from sd card: "));
        Serial.println(configFilePath);
        saved_calibration_file.read((uint8_t *)&onboard_config, sizeof(onboard_config) / sizeof(uint8_t)); // read the whole struct from the bytes on the sd card into the onboard_config struct in arduino memory.
        saved_calibration_file.close();                                                                    // close the file:
        // EEPROM_writeAnything(0, onboard_config);
      }
      else
      {
        print_sd_card_error(configFilePath); // if the file didn't open, print an error with the file path.
        loading_was_successful = false;
      }
    }
    else
    {
      Serial.print(F("CTD onboard_components calibration/config file was NOT found on sd card: "));
      Serial.println(configFilePath);
      loading_was_successful = false;
    }
  }
  else
  {
    loading_was_successful = false;
    Serial.println("Could not load the onboard_components calibration/config file because this feather board's serial number could not be determined");
  }

  // ----- Load config file for components attached to the ctd tube: -----
  if (strlen(highest_temp_probe_address_str) >= 16)
  { // < make sure the probe address was fully read (not less than 16 charaters)
    sprintf(configFilePath, "%stube_components_config-v%i-%s.bin", CONFIG_STORAGE_FOLDER_PATH, CONFIG_VERSION_NUMBER, highest_temp_probe_address_str);
    if (sd.exists(configFilePath))
    { // < make sure the config file exists
      saved_calibration_file.open(configFilePath, FILE_READ);
      if (saved_calibration_file)
      {
        Serial.print("Reading tube calibration file from sd card: ");
        Serial.println(configFilePath);
        saved_calibration_file.read((uint8_t *)&tube_components_config, sizeof(tube_components_config) / sizeof(uint8_t)); // read the whole struct from the bytes on the sd card into the tube_components_config struct in arduino memory.
        saved_calibration_file.close();                                                                                    // close the file.
      }
      else
      {
        print_sd_card_error(configFilePath); // if the file didn't open, print an error with the file path.
        loading_was_successful = false;
      }
    }
    else
    {
      Serial.print(F("CTD tube components calibration/config file was NOT found on sd card: "));
      Serial.println(configFilePath);
      loading_was_successful = false;
    }
  }
  else
  {
    loading_was_successful = false;
    Serial.println("Could not load the tube_components_config calibration/config file because the serial numbers of the temperature probes could not be determined!");
  }

  return loading_was_successful;
}

bool setup_calibration_storage()
{
  get_feather_board_unique_serial_number();
  Serial.print(F("Feather Board Serial Number: "));
  Serial.println(feather_board_serial_number);
  temp_get_highest_probe_address();
  Serial.print(F("Highest Found Temperature Probe Address: "));
  Serial.println(highest_temp_probe_address_str);
  // EEPROM_readAnything(0, saved_config); // old method (didn't work well)
  bool loadWasSuccessful = load_configs_from_sd_card();
  if (not loadWasSuccessful)
    Serial.println(F("\n!!! Failed to load one or more config files !!! \n - Is the CTD tube plugged in? \n - Have you calibrated this CTD yet? \n - Did you keep the same SD Card, Electronics Board and CTD tube used durring calibration? \n"));
  return loadWasSuccessful;
}

void save_calibration_storage()
{
  // EEPROM_writeAnything(0, saved_config); // old method (didn't work well)
  save_configs_to_sd_card();
}

void print_calibration_points(float *low_measured_value_storage_loc, float *low_true_value_storage_loc, float *high_measured_value_storage_loc, float *high_true_value_storage_loc)
{
  Serial.print(F("Current Calibration Values"));
  if (!isnan(*high_measured_value_storage_loc) and !isnan(*high_true_value_storage_loc) and !isnan(*low_measured_value_storage_loc) and !isnan(*low_true_value_storage_loc))
  {
    Serial.println(" | Sensor IS fully calibrated:");
  }
  else
  {
    Serial.println(" | Sensor NOT fully calibrated:");
  }

  Serial.print("High calib point: ");
  Serial.print(*high_measured_value_storage_loc);
  Serial.print(" => ");
  Serial.println(*high_true_value_storage_loc);
  Serial.print("Low calib point: ");
  Serial.print(*low_measured_value_storage_loc);
  Serial.print(" => ");
  Serial.println(*low_true_value_storage_loc);
}

void save_calibration_point(float measured_value, float true_value, float *measured_value_storage_loc, float *true_value_storage_loc)
{
  Serial.println("Saving Calibration Point...");
  *measured_value_storage_loc = measured_value;
  *true_value_storage_loc = true_value;
  save_calibration_storage();
  Serial.print(*measured_value_storage_loc);
  Serial.print(" => ");
  Serial.println(*true_value_storage_loc);
}

void save_calibration_point(float measured_value, float true_value, float *low_measured_value_storage_loc, float *low_true_value_storage_loc, float *high_measured_value_storage_loc, float *high_true_value_storage_loc, bool *sensor_is_calibrated_storage_loc)
{
  // this function decides whether the new point goes in the high vs low position, the true_value is the value that the sensor should report / the calibrated value
  Serial.println("Saving Calibration Point...");

  if (isnan(*low_true_value_storage_loc))
  { // calibration config values start as nan (for not a number), so this checks if the low real/calibrated value hasn't been written to yet
    *low_measured_value_storage_loc = measured_value;
    *low_true_value_storage_loc = true_value;
  }
  else if (isnan(*high_true_value_storage_loc))
  { // calibration config values start as nan (for not a number), so this checks if the high real/calibrated value hasn't been written to yet
    *high_true_value_storage_loc = true_value;
    *high_measured_value_storage_loc = measured_value;
  }
  else if (true_value > *low_true_value_storage_loc)
  {
    *high_true_value_storage_loc = true_value;
    *high_measured_value_storage_loc = measured_value;
    *low_true_value_storage_loc = NAN;
  }
  else if (true_value < *high_true_value_storage_loc)
  {
    *low_measured_value_storage_loc = measured_value;
    *low_true_value_storage_loc = true_value;
    *high_true_value_storage_loc = NAN;
  }
  // TODO! handle the case where low is greater yhan high

  if (not isnan(*high_measured_value_storage_loc) and !isnan(*high_true_value_storage_loc) and !isnan(*low_measured_value_storage_loc) and !isnan(*low_true_value_storage_loc))
    *sensor_is_calibrated_storage_loc = true;
  print_calibration_points(low_measured_value_storage_loc, low_true_value_storage_loc, high_measured_value_storage_loc, high_true_value_storage_loc, sensor_is_calibrated_storage_loc);
  save_calibration_storage();
}

void reset_config()
{
  onboard_config.seconds_between_log_events = 1.0;
  onboard_config.debugging_mode_on = true;
  onboard_config.log_raw_values = false;
  save_calibration_storage();
}

// constants from: https://gist.github.com/ah01/762576
#if defined(__AVR_ATmega328P__) // ----------------------------------------------------------------------------------------
// ^ This means we are using an arduino uno (or a board with the same processor)

#include <EEPROM.h>

word[4] get_arduino_uno_board_unique_serial_number()
{
  // https://www.thethingsnetwork.org/forum/t/arduino-has-a-unique-id/21415
}
#endif

// code from: https://playground.arduino.cc/Code/EEPROMWriteAnything/
template <class T>
int EEPROM_writeAnything(int ee, const T &value)
{
  const byte *p = (const byte *)(const void *)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
  return i;
}

template <class T>
int EEPROM_readAnything(int ee, T &value)
{
  byte *p = (byte *)(void *)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
  return i;
}

// code from: https://stackoverflow.com/questions/52122263/c-union-of-a-float-and-a-byte-array-issue
void WriteFloatToEEPROM(int address, float value)
{
  byte byteVal[sizeof(float)];
  memcpy(byteVal, &value, sizeof(float));

  for (int i = 0; i < sizeof(float); i++)
  {
    EEPROM.update(address + i, byteVal[i]);
  }
}

float ReadFloatFromEEPROM(int address)
{
  byte byteVal[sizeof(float)];

  for (int i = 0; i < sizeof(float); i++)
  {
    byteVal[i] = EEPROM.read(address + i);
  }

  float f;
  memcpy(&f, byteVal, sizeof(float));
  return f;
}

#endif

#endif
