#include "temp_probe.hpp"
#if ENABLE_TEMP_PROBE


/* -------------------------------------------*/
/* ----- Global Variables -------*/
/* -----------------------------------------*/

OneWire oneWire(TEMP_SENSOR_DATA_PIN);     // Define the OneWire port for temperature.
DallasTemperature temp_sensors(&oneWire);  // Define DallasTemperature input based on OneWire.

ulong tempDelayStartTime;  // A "unsigned long" number to mark when we requested a temperature mesurement so we can wait the required delay before reading each value.
ulong requiredMesurementDelay = temp_sensors.millisToWaitForConversion(TEMP_SENSOR_RESOLUTION);

float temp_values[NUM_TEMP_PROBES];
SensorCalibration_t temp_sensors_calibrations[NUM_TEMP_PROBES];

// -------------------------------------
// -------------- Functions -----------
// -------------------------------------

void temp_setup_sensors() {
  temp_sensors.begin();                                // Intialize the temperature sensors.
  temp_sensors.setResolution(TEMP_SENSOR_RESOLUTION);  // Set the resolution (accuracy) of the temperature sensors.
  temp_sensors.requestTemperatures();                  // on the first pass request all temperatures in a blocking way to start the variables with true data.
  for (int i = 0; i < NUM_TEMP_PROBES; i++) {
    temp_values[i] = temp_sensors.getTempCByIndex(i);
    temp_sensors_calibrations[i] = read_sensor_calibration((String("temp_") + String(i)).c_str(), false);
  }
  temp_sensors.setWaitForConversion(false);  // Now tell the Dallas Temperature library to not block this script while it's waiting for the temperature mesurement to happen
}

void temp_probes_refresh_values() {
  //
  if (millis() - tempDelayStartTime > requiredMesurementDelay) {
    // Read the temperature sensors.
    for (int i = 0; i < NUM_TEMP_PROBES; i++) {
      temp_values[i] = temp_sensors.getTempCByIndex(i);
    }
    tempDelayStartTime = millis();
    temp_sensors.requestTemperatures();  // request all temp probes make a new measurement
  }
}

float temp_get_latest_raw_value_by_probe_index(int probe_indx) {
  if (temp_values[probe_indx] == DEVICE_DISCONNECTED_C) {
    return NAN;  // Return Not a Number (NAN) to indicate temperature probe has error or is disconnected.
  } else {
    return temp_values[probe_indx];  // Return the most recent measured value from that probe.
  }
}

bool temp_is_calibrated() {
  for (size_t i = 0; i < NUM_TEMP_PROBES; i++) {
    if (temp_sensors_calibrations[i].length == 0)
      return false;
  }
  return true;
}

float temp_get_latest_calibrated_value_by_probe_index(int probe_indx) {
  // Return the most recent temp value from that probe.
  float raw_temp = temp_get_latest_raw_value_by_probe_index(probe_indx);
  if (temp_is_calibrated() and !onboard_config.log_raw_values) {
    return calculate_calibrated_value(raw_temp, &temp_sensors_calibrations[probe_indx]);
  } else {
    return raw_temp;
  }
}

void temp_log_value(uint8_t probe_indx) {
  Serial.print(F("Temp"));
  Serial.print(probe_indx);
  if (temp_is_calibrated() and !onboard_config.log_raw_values) {
    Serial.print(F("_c:"));
    sd_log_value(datalogFile, temp_get_latest_calibrated_value_by_probe_index(0));
  } else {
    Serial.print(F("_c_RAW:"));
    sd_log_value(datalogFile, temp_get_latest_raw_value_by_probe_index(0));
  }
}

// bool temp_check_for_probe_address_match(DeviceAddress known_probe_address) {
//   // Checks if any of the currently connected temperature probes has a Unique Address Number that matches the one in known_probe_address.
//   // (Used to verify that the calibration values stored in memory go with this CTD tube, and no switching of board/tube combos happened)
//   DeviceAddress temp_probe_address;
//   for (int i = 0; i < NUM_TEMP_PROBES; i++) {
//     if (not temp_sensors.getAddress(temp_probe_address, 0)) {
//       Serial.print("Unable to find address for Temp Probe "); Serial.println(i);
//     }
//     if (memcmp(temp_probe_address, known_probe_address, sizeof(DeviceAddress)) == 0) return true;
//   }
//   return false; // if we got here none
// }

bool temp_get_probe_address(DeviceAddress *temp_probe_address_destination, uint8_t probe_indx) {
  // Puts the first currently connected temperature probe Unique Address Number into the temp_probe_address_destintation passed variable.
  bool success = temp_sensors.getAddress(*temp_probe_address_destination, probe_indx);
  if (not success) {
    Serial.print("Unable to find address for Temp Probe at index ");
    Serial.println(probe_indx);
  }
  return success;
}

bool temp_get_probe_address_str(char *temp_probe_address_str_destination, uint8_t probe_indx) {
  // Puts the first currently connected temperature probe Unique Address Number into the temp_probe_address_destintation passed variable.
  DeviceAddress temporary_probe_adddress_variable;
  bool success = temp_sensors.getAddress(temporary_probe_adddress_variable, probe_indx);
  if (success) {
    // print out all bytes in address as hexadecimal
    for (int j = 0; j < 8; j++) {
      sprintf(&temp_probe_address_str_destination[2 * j], "%02X", temporary_probe_adddress_variable[j]);
    }
    temp_probe_address_str_destination[16] = '\0';  // add the line terminator character at the end
    // Serial.println(temp_probe_address_str_destination); // print out the finished address string
  }
  return success;
}

char *temp_get_highest_probe_address() {
  // Finds the highest temp probe Address numerically.
  // Puts the higest temperature probe Unique Address Number into the temp_probe_address_str_output passed variable as a string.
  // since the temp probe Addresses are numbers, we can compare them to get the one with the highest value, which should always be the same given the same 3 temp probes in a tube.

  // start with the first probe address:
  temp_get_probe_address_str(highest_temp_probe_address_str, 0);

  // then loop through the following probes to find the max one:
  char probe_address_str[17];
  for (int i = 1; i < NUM_TEMP_PROBES; i++) {
    bool sucessfully_got_address = temp_get_probe_address_str(probe_address_str, i);
    if (sucessfully_got_address and strcmp(probe_address_str, highest_temp_probe_address_str) > 0) {
      sprintf(highest_temp_probe_address_str, "%s", probe_address_str);
    }
  }
  return highest_temp_probe_address_str;
}

void temp_command_mode_loop() {
  Serial.print(F("Entering Temperature Probes Calibration Mode... "));
  const __FlashStringHelper *cmd_help_mssg = F("\n\n----- Available Commands ------\n"
                                               "       | Saves a calibration point for all probes for the current temperature.\n"
                                               "cp     | Command format: cp,21.34 \n"
                                               "       | Here 21.34 is the known temperature around all probes  \n"
                                               "       | in Celcius when you entered the cp command. \n"
                                               "-------+----------------------------------------------- \n"
                                               "clear  | Deletes all saved calibration values for all temp probes \n"
                                               "-------+----------------------------------------------- \n"
                                               "q      | quit/exit this mode.\n"
                                               "---------------------------------------------------------- \n\n");

  // print saved calibration data:
  for (int i = 0; i < NUM_TEMP_PROBES; i++) {
    Serial.print("Temp Probe ");
    Serial.print(i);
    Serial.print(" | ");
    print_calibration_values(&temp_sensors_calibrations[i]);
  }

  // command loop
  latest_full_cmd[0] = '\0';  // clear the latest full command so that it a: won't accidentally trigger a named command below, b: will trigger the cmd_help_mssg to show because the command is unknown.
  while (true) {

    power_ctrl_check_mag_switch();

    // every second print the raw measured value:
    if (millis() % 1000 == 0) {
      // every second print the raw measured values:
      temp_probes_refresh_values();
      for (int i = 0; i < NUM_TEMP_PROBES; i++) {
        float temp = temp_get_latest_raw_value_by_probe_index(i);
        Serial.print(F("Temp_"));
        Serial.print(i);
        Serial.print(F("_c_RAW:"));
        Serial.print(temp, 5);
        Serial.print(",");
      }
      Serial.println();
    }
    // check for new characters from serial monitor (user's computer):
    read_serial_input_characters();
    if (!cmd_ready) continue;

    // handle the command:
    parse_command();
    if (strcmp(latest_full_cmd, "q") == 0)
      break;
    if (strcmp(latest_cmd_str, "cp") == 0) {
      for (int i = 0; i < NUM_TEMP_PROBES; i++) {
        Serial.print("Saving Temp Probe ");
        Serial.print(i);
        Serial.print(" | ");
        add_calibration_point(temp_get_latest_raw_value_by_probe_index(i), latest_cmd_value_1, &temp_sensors_calibrations[i]);
        write_sensor_calibration((String("temp_") + String(i)).c_str(), false, &temp_sensors_calibrations[i]);
        print_calibration_values(&temp_sensors_calibrations[i]);
      }
    } else if (strcmp(latest_cmd_str, "cpl") == 0) {
      for (int i = 0; i < NUM_TEMP_PROBES; i++) {
        Serial.print("Saving Temp Probe ");
        Serial.print(i);
        Serial.print(" | ");
        add_calibration_point(latest_cmd_value_1, latest_cmd_value_2, &temp_sensors_calibrations[i]);
        write_sensor_calibration((String("temp_") + String(i)).c_str(), false, &temp_sensors_calibrations[i]);
        print_calibration_values(&temp_sensors_calibrations[i]);
      }
    } else if (strcmp(latest_cmd_str, "clear") == 0) {
      for (int i = 0; i < NUM_TEMP_PROBES; i++) {
        Serial.print("Clearing Temp Probe ");
        Serial.print(i);
        Serial.print(" | ");
        clear_sensor_calibration(&temp_sensors_calibrations[i]);
        write_sensor_calibration((String("temp_") + String(i)).c_str(), false, &temp_sensors_calibrations[i]);
        print_calibration_values(&temp_sensors_calibrations[i]);
      }
    } else {
      Serial.println(cmd_help_mssg);
    }
  }
}

#endif
