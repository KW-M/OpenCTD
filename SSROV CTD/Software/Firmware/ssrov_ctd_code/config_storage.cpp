#include "config_storage.hpp"
//#include "Adafruit_USBD_CDC.h"
#include "power_control.hpp"
#include "utility_functions.hpp"
#include "sdcard.hpp"

// this file requires these libries from arduino library manager
// - ArduinoSTL by Mike Matera (used version 1.3.3)
// - ArduinoStreamUtils by Benoit Blanchon
// - ArduinoJson by Benoit Blanchon (used version 6.19.3)
// - MultiMap by Rob Tillaart

onboard_config_type default_onboard_config = {};
onboard_config_type onboard_config = {};

char feather_board_serial_number[33];
char highest_temp_probe_address_str[17];

// -------------------------------------
// -------------- Functions -----------
// -------------------------------------

#if defined(ARDUINO_ARCH_SAMD) // ------------- < This means we are using a feather m0 (or a board with the same processor)
char *get_feather_serial_number()
{
  /* Each feather device has a unique 128-bit serial number which is a concatenation of four 32-bit words at the following addresses: */
  static long unsigned int serial_num[4]{
      /* Word 0 address: 0x0080A00C */ pgm_read_dword(0x0080A00C), //: this function reads a 32 bit word from flash memory (found here: https://www.nongnu.org/avr-libc/user-manual/group__avr__pgmspace.html#ga88d7dd4863f87530e1a34ece430a587c)
      /* Word 1 address: 0x0080A040 */ pgm_read_dword(0x0080A040),
      /* Word 2 address: 0x0080A044 */ pgm_read_dword(0x0080A044),
      /* Word 3 address: 0x0080A048 */ pgm_read_dword(0x0080A048)};
  // save the serial number as as a hexadecimal string to the global variable feather_board_serial_number
  sprintf(feather_board_serial_number, "%08lX%08lX%08lX%08lX", serial_num[0], serial_num[1], serial_num[2], serial_num[3]);
  return feather_board_serial_number;
}
#endif // ----------------------------------------------------------------------------------------

/**
 * @param doc the parsed arduinoJson doc to copy values from
 * @return true if any of the config keys were set to defaults, and should be written back to the sd card.
 */
bool json_config_to_struct(JsonDocument &doc)
{
  onboard_config.log_raw_values = doc.containsKey("log_raw_values") ? doc["log_raw_values"].as<bool>() : default_onboard_config.log_raw_values;
  onboard_config.seconds_between_log_events = doc.containsKey("seconds_between_log_events") ? doc["seconds_between_log_events"].as<float>() : default_onboard_config.seconds_between_log_events;
  onboard_config.max_seconds_to_resume_writing_to_previous_logfile_after_reboot = doc.containsKey("max_seconds_to_resume_writing_to_previous_logfile_after_reboot") ? doc["max_seconds_to_resume_writing_to_previous_logfile_after_reboot"].as<int>() : default_onboard_config.max_seconds_to_resume_writing_to_previous_logfile_after_reboot;
  if (doc.containsKey("last_datalog_file_path") && doc["last_datalog_file_path"].as<String>().length() > 0)
  {
    strcpy(onboard_config.last_datalog_file_path, doc["last_datalog_file_path"].as<String>().c_str());
  }

  if (!doc.containsKey("log_raw_values") || !doc.containsKey("seconds_between_log_events") || !doc.containsKey("max_seconds_to_resume_writing_to_previous_logfile_after_reboot") || !doc.containsKey("last_datalog_file_path"))
  {
    return true; // new config values were set (to struct in memory that are not yet saved to SD card)
  }
  else
  {
    return false; // new config values were not set
  }
}

/**
 * @param doc an empty arduinoJson doc reference to put values into
 */
void onboard_struct_to_json_config(JsonDocument &doc)
{
  doc["log_raw_values"] = onboard_config.log_raw_values;
  doc["seconds_between_log_events"] = onboard_config.seconds_between_log_events;
  doc["max_seconds_to_resume_writing_to_previous_logfile_after_reboot"] = onboard_config.max_seconds_to_resume_writing_to_previous_logfile_after_reboot;
  doc["last_datalog_file_path"] = onboard_config.last_datalog_file_path;
}

String get_onboard_config_file_path()
{
  return String(CONFIG_STORAGE_FOLDER_PATH) + "onboard-config-" + feather_board_serial_number + ".json";
}

String get_tube_config_file_path()
{
  return String(CONFIG_STORAGE_FOLDER_PATH) + "tube-config-" + highest_temp_probe_address_str + ".json";
}

void write_onboard_config()
{
  println("Writing onboard config...");

  // make sure the config storage folder is created:
  sd_create_folder(CONFIG_STORAGE_FOLDER_PATH);

  // Open the file for writing
  File32 configFile;
  String configFilePath = get_onboard_config_file_path();
  bool openSuccessful = sd_open_file(configFile, configFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
  if (not openSuccessful)
  {
    Serial.println(String("Failed to open file for write: ") + configFilePath + " - changes NOT saved.");
    return;
  }

  // Read the current config values from the struct into a json doc
  StaticJsonDocument<412> doc;
  onboard_struct_to_json_config(doc);

  // Output as a JSON text file
  WriteBufferingStream bufferedFile(configFile, 512);
  serializeJsonPretty(doc, bufferedFile);
  bufferedFile.flush();
  configFile.close();
}

void read_onboard_config()
{
  StaticJsonDocument<412> doc;
  String configFilePath = get_onboard_config_file_path();
  bool newConfigValuesWereSet = false;

  // open the file
  File32 configFile;
  bool openSuccessful = sd_open_file(configFile, configFilePath.c_str(), O_RDONLY);

  // If the file does not exist or cannot be opened, write the default config and return
  if (not openSuccessful)
  {
    println("Failed to open config file: " + configFilePath + " - using default config values");
    onboard_config = default_onboard_config;
    write_onboard_config();
    return;
  }

  // Parse the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  // Test if parsing succeeds.
  if (error)
  {
    print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
  }
  else
  {
    // copy over the config values from the json doc
    newConfigValuesWereSet = json_config_to_struct(doc);
  }

  println("Config read.");

  if (newConfigValuesWereSet)
  {
    write_onboard_config();
  }
}

void setup_config_storage()
{
  print(F("Feather Serial Number: "));
  Serial.println(get_feather_serial_number());

  print(F("Greatest Temp Probe Addr: "));
  Serial.println(temp_get_highest_probe_address());

  read_onboard_config();
  // bool loadWasSuccessful = readOnboardConfig();
  // if (not loadWasSuccessful)
  // {
  //   println(F("\n!!! Failed to read onboard config file !!! \n - Is the CTD tube plugged in? \n - Have you calibrated this CTD yet? \n - Did you keep the same SD Card; Electronics Board; and CTD tube used durring calibration? \n"));
  // }
}

void write_sensor_calibration(String jsonKey, bool onboardComponent, SensorCalibration_t *sensorCalibration)
{
  // open the file
  File32 configFile;
  StaticJsonDocument<412> doc;
  String configFilePath = onboardComponent ? get_onboard_config_file_path() : get_tube_config_file_path();
  bool openSuccessful = sd_open_file(configFile, configFilePath.c_str(), O_RDONLY);
  if (openSuccessful)
  {
    // Parse the full current JSON document:
    DeserializationError error = deserializeJson(doc, configFile);

    // Test if parsing succeeds.
    if (error)
    {
      Serial.println(String("deserializeJson() failed: ") + error.f_str() + +" - changes NOT saved.");
    }
  }

  // Add the other config values if the onboard component config file is being written:
  if (onboardComponent)
  {
    onboard_struct_to_json_config(doc);
  }

  // copy over the new sensor calibration to the json doc:
  JsonObject sensorConfJson = doc.createNestedObject(jsonKey);
  sensorConfJson["length"] = sensorCalibration->length;
  JsonArray mesuredValuesJson = sensorConfJson.createNestedArray("measuredValues");
  JsonArray realValuesJson = sensorConfJson.createNestedArray("realValues");
  for (int i = 0; i < sensorCalibration->length; i++)
  {
    mesuredValuesJson.add(sensorCalibration->measuredValues[i]);
    realValuesJson.add(sensorCalibration->realValues[i]);
  }

  // Output as a JSON text file
  openSuccessful = sd_open_file(configFile, configFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
  if (not openSuccessful)
  {
    Serial.println(String("Failed to create file: ") + configFilePath + " - changes NOT saved.");
    return;
  }
  WriteBufferingStream bufferedFile(configFile, 512);
  serializeJsonPretty(doc, bufferedFile);
  bufferedFile.flush();
  configFile.close();
}

void clear_sensor_calibration(SensorCalibration_t *sensorCalibration)
{
  sensorCalibration->length = 0;
  sensorCalibration->measuredValues = new float[0];
  sensorCalibration->realValues = new float[0];
}

SensorCalibration_t read_sensor_calibration(String jsonKey, bool onboardComponent)
{
  SensorCalibration_t sensorCalibration;
  sensorCalibration.length = 0;
  sensorCalibration.measuredValues = new float[0];
  sensorCalibration.measuredValues = new float[0];

  StaticJsonDocument<412> doc;

  // open the file
  File32 configFile;
  String configFilePath = onboardComponent ? get_onboard_config_file_path() : get_tube_config_file_path();
  bool fileReadable = sd_open_file(configFile, configFilePath.c_str(), O_RDONLY);
  if (not fileReadable)
  {
    println("Failed to open config file: " + configFilePath + " - using default config values");
    clear_sensor_calibration(&sensorCalibration);
    write_sensor_calibration(jsonKey, onboardComponent, &sensorCalibration);
    return sensorCalibration;
  }
  else
  {

    // Parse the JSON document
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    // Test if parsing succeeds.
    if (error)
    {
      print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      clear_sensor_calibration(&sensorCalibration);
      return sensorCalibration;
    }
    else if (doc.containsKey(jsonKey))
    {
      JsonArray measuredValues = doc["measuredValues"];
      JsonArray realValues = doc["realValues"];
      sensorCalibration.length = realValues.size();
      sensorCalibration.measuredValues = new float[sensorCalibration.length];
      sensorCalibration.realValues = new float[sensorCalibration.length];
      for (int i = 0; i < sensorCalibration.length; i++)
      {
        sensorCalibration.measuredValues[i] = measuredValues[i];
        sensorCalibration.realValues[i] = realValues[i];
      }
    }
    else
    {
      print(F("- calibration json not found: "));
      Serial.println(jsonKey);
    }
  }
  // print(F("return calibration length: "));
  // Serial.println(sensorCalibration.length);
  return sensorCalibration;
}

void add_calibration_point(float measuredValue, float realValue, SensorCalibration_t *sensorCalibration)
{
  float *new_measuredValues = new float[sensorCalibration->length + 1];
  float *new_realValues = new float[sensorCalibration->length + 1];

  // insert the calibration point into the new_measuredValues and new_realValues array at the sorted position and copy over all other values
  int indexOffset = 0;
  int i = 0;
  for (; i < sensorCalibration->length; i++)
  {
    if (indexOffset == 0 && measuredValue < sensorCalibration->measuredValues[i])
    {
      new_measuredValues[i] = measuredValue;
      new_realValues[i] = realValue;
      indexOffset++;
    }
    new_measuredValues[i + indexOffset] = sensorCalibration->measuredValues[i];
    new_realValues[i + indexOffset] = sensorCalibration->realValues[i];
  }
  if (indexOffset == 0)
  {
    new_measuredValues[i] = measuredValue;
    new_realValues[i] = realValue;
  }
  sensorCalibration->measuredValues = new_measuredValues;
  sensorCalibration->realValues = new_realValues;
  sensorCalibration->length += 1;
}

float calculate_calibrated_value(float measuredValue, SensorCalibration_t *sensorCalibration)
{
  if (isnan(measuredValue))
    return NAN;
  else if (isnan(sensorCalibration->length) || sensorCalibration->length == 0)
    return measuredValue;
  else
    return multiMap(measuredValue, sensorCalibration->measuredValues, sensorCalibration->realValues, sensorCalibration->length);
}

void print_calibration_values(SensorCalibration_t *sensorCalibration)
{
  print("Calibration: ");
  if (sensorCalibration->length > 0)
  {
    for (int i = 0; i < sensorCalibration->length; i++)
    {
      Serial.print(sensorCalibration->measuredValues[i]);
      print("->");
      Serial.print(sensorCalibration->realValues[i]);
      print(" | ");
    }
  }
  else
  {
    print("Not Calibrated.");
  }
  Serial.println();
}
