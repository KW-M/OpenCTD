#if ENABLE_PHOTORESISTOR
#include "photoresistor.hpp"

// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

SensorCalibration_t photoresistor_calibration;

// ----------------------------------------
// -------- Photoresitor Functions --------
// ----------------------------------------

void photoresistor_setup_sensor() {
  photoresistor_calibration = readSensorCalibration("photoresistor", true);
}

float photoresistor_get_raw_light_level() {
  digitalWrite(PHOTORESISTOR_ENABLE_PIN, HIGH);  // Turn on the photoresistor circut

  float light_level = 0.0;
  // Take a bunch of mesurements from the photoresistor and average them together:
  for (int i = 0; i < PHOTORESISTOR_SAMPLE_AVERAGING_COUNT; i++) {
    light_level += analogRead(PHOTORESISTOR_SENSE_PIN) / 1024.0;  // divide by 1024 because the maximum value reported by the analog input is 1024.
  }
  light_level /= PHOTORESISTOR_SAMPLE_AVERAGING_COUNT;  // divide by the number of samples to get the average light level over all samples.

  digitalWrite(PHOTORESISTOR_ENABLE_PIN, LOW);  // Turn off the photoresistor circuit
  return light_level;
}

bool photoresistor_is_calibrated() {
  return photoresistor_calibration.length > 0;
}

float photoresistor_get_lux_light_level() {
  float raw_light_lvl = photoresistor_get_raw_light_level();
  return calculate_calibrated_value(raw_light_lvl, &photoresistor_calibration);
}

void photoresistor_log_value() {
  if (photoresistor_is_calibrated() and !onboard_config.log_raw_values) {
    Serial.print(F("LightLvl_lux:"));
    sd_log_value(datalogFile, photoresistor_get_lux_light_level());
  } else {
    Serial.print(F("LightLvl_0to1_RAW:"));
    sd_log_value(datalogFile, photoresistor_get_raw_light_level());
  }
}

void photoresistor_command_mode_loop() {
  Serial.print(F("Entering Light Sensor Command Mode... "));
  const __FlashStringHelper *cmd_help_mssg = F("\n\n----- Available Commands ------ \n"
                                               "       | Saves a calibration point for the sensor with the current light level.\n"
                                               "cp     | Command format: cp,76 \n"
                                               "       | Here 76 is the known amount of light hitting \n"
                                               "       | the sensor in lux when you entered the cp command. \n"
                                               "-------+----------------------------------------------- \n"
                                               "       | Saves a linear calibration point for the sensor when you have a pre-measured value.\n"
                                               "cpl    | Command format: cpl,0.1,76 \n"
                                               "       | Here 0.1 was the value that the light sensor reported when exposed to 76 lux. \n"
                                               "-------+----------------------------------------------- \n"
                                               "clear  | Deletes all saved calibration values for this sensor \n"
                                               "-------+----------------------------------------------- \n"
                                               "q      | quit / exit this mode.\n"
                                               "---------------------------------------------------------- \n\n");
  print_calibration_values(&photoresistor_calibration);
  latest_full_cmd[0] = '\0';  // clear the latest full command so that it a: won't accidentally trigger a named command below, b: will trigger the cmd_help_mssg to show because the command is unknown.
  while (true) {


    power_ctrl_check_mag_switch();

    // every second print the raw measured value:
    if (millis() % 1000 == 0) {
      float light_level = photoresistor_get_raw_light_level();
      Serial.print("Raw_light_level_0_to_1:");
      Serial.println(light_level, 5);
    }

    // check for new characters from serial monitor (user's computer):
    read_serial_input_characters();
    if (!cmd_ready) continue;

    // handle the command:
    parse_command();
    if (strcmp(latest_full_cmd, "q") == 0)
      break;
    if (strcmp(latest_cmd_str, "cp") == 0) {
      add_calibration_point(photoresistor_get_raw_light_level(), latest_cmd_value_1, &photoresistor_calibration);
      writeSensorCalibration("photoresistor", true, &photoresistor_calibration);
      print_calibration_values(&photoresistor_calibration);
    } else if (strcmp(latest_cmd_str, "cpl") == 0) {
      add_calibration_point(latest_cmd_value_1, latest_cmd_value_2, &photoresistor_calibration);
      writeSensorCalibration("photoresistor", true, &photoresistor_calibration);
      print_calibration_values(&photoresistor_calibration);
    } else if (strcmp(latest_cmd_str, "clear") == 0) {
      clearSensorCalibration(&photoresistor_calibration);
      writeSensorCalibration("photoresistor", true, &photoresistor_calibration);
      print_calibration_values(&photoresistor_calibration);
    } else {
      Serial.println(cmd_help_mssg);
    }
  }
}


#endif
