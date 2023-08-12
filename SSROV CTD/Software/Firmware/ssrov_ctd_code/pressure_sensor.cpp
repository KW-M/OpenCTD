#include "pressure_sensor.hpp"
#if ENABLE_PRESSURE_SENSOR


/* -------------------------------------------*/
/* ----- Global Variables -------*/
/* -----------------------------------------*/

MS_5803 pressure_sensor = MS_5803(PRESSURE_SENSOR_RESOLUTION);  // Define pressure pressure_sensor.
SensorCalibration_t pressure_sensor_calibration = {};

// ----------------------------------------
// -------- Presure Sensor Functions --------
// ----------------------------------------

void pressure_setup_sensor() {
  // Initialize pressure sensor
  Serial.println(F("---- Pressure Sensor Info: ----"));
  pressure_sensor.initializeMS_5803();  // Initialize pressure sensor
  Serial.println(F("--------------------------- NOTE: If all the above values are 0, the pressure sensor is not hooked up correctly."));
  pressure_sensor_calibration = read_sensor_calibration("pressure", false);
}

bool pressure_sensor_is_calibrated() {
  return pressure_sensor_calibration.length > 0;
}

float pressure_get_raw_reading() {
  pressure_sensor.readSensor();       // read pressure sensor
  return pressure_sensor.pressure();  // return the absolute pressure
}

float pressure_get_calibrated_reading() {
  float raw_pressure = pressure_get_raw_reading();
  if (raw_pressure == 0.0) {  // we will assume that the ctd is never in outer space, so zero pressure is imposible, so a value of zero just means the sensor is disconnected (we return Not A Number).
    return NAN;
  } else if (pressure_sensor_is_calibrated() and !onboard_config.log_raw_values) {
    return calculate_calibrated_value(raw_pressure, &pressure_sensor_calibration);
  } else {
    return raw_pressure;
  }
}

void pressure_log_value() {
  if (pressure_sensor_is_calibrated() and !onboard_config.log_raw_values) {
    Serial.print(F("Pressure_mBar:"));
    sd_log_value(datalogFile, pressure_get_calibrated_reading());
  } else {
    Serial.print(F("Pressure_mBar_RAW:"));
    sd_log_value(datalogFile, pressure_get_raw_reading());
  }
}

void pressure_command_mode_loop() {
  Serial.print(F("Entering Pressure Sensor Calibration Mode... "));
  const __FlashStringHelper *cmd_help_mssg = F("\n\n----- Available Commands ------\n"
                                               "       | Saves a calibration point for the sensor with the current pressure.\n"
                                               "cp     | Command format: cp,1096 \n"
                                               "       | Here 1096 is the known amount of pressure on the sensor  \n"
                                               "       | in milipascals when you entered the cp command. \n"
                                               "-------+----------------------------------------------- \n"
                                               "       | Saves a linear calibration point for the sensor when you have a pre-measured value.\n"
                                               "cpl    | Command format: cpl,998,1010 \n"
                                               "       | Here 998 was the value that the sensor reported when exposed  \n"
                                               "       | to 1010 milipascals of pressure. \n"
                                               "-------+----------------------------------------------- \n"
                                               "clear  | Deletes all saved calibration values for this sensor \n"
                                               "-------+----------------------------------------------- \n"
                                               "q      | quit / exit this mode.\n"
                                               "---------------------------------------------------------- \n\n");

  // print saved calibration data:
  print_calibration_values(&pressure_sensor_calibration);
  latest_full_cmd[0] = '\0';  // clear the latest full command so that it a: won't accidentally trigger a named command below, b: will trigger the cmd_help_mssg to show because the command is unknown.

  // command loop
  while (true) {

    power_ctrl_check_mag_switch();

    // every second print the raw measured value:
    if (millis() % 1000 == 0) {
      float pressure = pressure_get_raw_reading();
      Serial.print("Raw_Pressure_mBar:");
      Serial.print(pressure, 5);
      Serial.println(",");
    }

    // check for new characters from serial monitor (user's computer):
    read_serial_input_characters();
    if (!cmd_ready) continue;


    parse_command();
    if (strcmp(latest_full_cmd, "q") == 0)
      break;
    if (strcmp(latest_cmd_str, "cp") == 0) {
      add_calibration_point(pressure_get_raw_reading(), latest_cmd_value_1, &pressure_sensor_calibration);
      write_sensor_calibration("pressure", false, &pressure_sensor_calibration);
      print_calibration_values(&pressure_sensor_calibration);
    } else if (strcmp(latest_cmd_str, "cpl") == 0) {
      add_calibration_point(latest_cmd_value_1, latest_cmd_value_2, &pressure_sensor_calibration);
      write_sensor_calibration("pressure", false, &pressure_sensor_calibration);
      print_calibration_values(&pressure_sensor_calibration);
    } else if (strcmp(latest_cmd_str, "clear") == 0) {
      clear_sensor_calibration(&pressure_sensor_calibration);
      write_sensor_calibration("pressure", false, &pressure_sensor_calibration);
      print_calibration_values(&pressure_sensor_calibration);
    } else {
      Serial.println(cmd_help_mssg);
    }
  }
}

#endif
