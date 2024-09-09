#include "pressure_sensor.hpp"
#if ENABLE_PRESSURE_SENSOR
#include "utility_functions.hpp"
#include "power_control.hpp"
#include "command_mode.hpp"
#include "config_storage.hpp"
#include "sdcard.hpp"

#include "SparkFun_MS5803_I2C.h"
/* -------------------------------------------*/
/* ----- Global Variables -------*/
/* -----------------------------------------*/

// MS_5803 pressure_sensor = MS_5803(PRESSURE_SENSOR_RESOLUTION);

MS5803 pressure_sensor(ADDRESS_HIGH); // Define pressure pressure_sensor.
SensorCalibration_t pressure_sensor_calibration = {};

// ----------------------------------------
// -------- Presure Sensor Functions --------
// ----------------------------------------

void pressure_setup_sensor()
{
  // Initialize pressure sensor
  // println(F("---- Pressure Sensor Info: ----"));
  // Retrieve calibration constants for conversion math.
  pressure_sensor.reset();
  pressure_sensor.begin(Wire, ms5803_addr::ADDRESS_HIGH);

  // pressure_sensor.initializeMS_5803();  // Initialize pressure sensor
  // println(F("--------------------------- NOTE: If all the above values are 0 then the pressure sensor is not hooked up correctly."));
  pressure_sensor_calibration = read_sensor_calibration("pressure", false);
}

bool pressure_sensor_is_calibrated()
{
  return pressure_sensor_calibration.length > 0;
}

float pressure_get_raw_reading()
{
  // pressure_sensor.readSensor();       // read pressure sensor
  return pressure_sensor.getPressure(ADC_4096); // return the absolute pressure in high resolution (4096 steps)
}

float pressure_get_calibrated_reading()
{
  float raw_pressure = pressure_get_raw_reading();
  if (raw_pressure == 0.0)
  { // we will assume that the ctd is never in outer space, so zero pressure is imposible, so a value of zero just means the sensor is disconnected (we return Not A Number).
    return NAN;
  }
  else if (pressure_sensor_is_calibrated() and !onboard_config.log_raw_values)
  {
    return calculate_calibrated_value(raw_pressure, &pressure_sensor_calibration);
  }
  else
  {
    return raw_pressure;
  }
}

float pressure_get_value()
{
  if (pressure_sensor_is_calibrated() and !onboard_config.log_raw_values)
  {
    return pressure_get_calibrated_reading();
  }
  else
  {
    return pressure_get_raw_reading();
  }
}

void pressure_sensor_show_live_data()
{
  // every second print the raw measured value:
  if (millis() % 1000 == 0)
  {
    float pressure = pressure_get_raw_reading();
    print_plot_value("Raw_Pressure_mBar",pressure);
    if (pressure_sensor_calibration.length > 0)
    {
      float calibrated_value = pressure_get_calibrated_reading();
      print_plot_value("Calibrated_Pressure_mBar",calibrated_value);
    }
    println("");
  }
}

bool pressure_sensor_user_command_handler(UserCommand latest_command, const __FlashStringHelper *general_cmd_mode_help)
{
  const __FlashStringHelper *cmd_help_msg = F("+------- Pressure Sensor Calibration Mode Commands -------+\n"
                                              "+------+--------------------------------------------------+\n"
                                              "       | Saves a calibration point for the sensor with the current pressure.\n"
                                              "cp     | Command format: cp;1096 \n"
                                              "       | Here 1096 is the known amount of pressure on the sensor  \n"
                                              "       | in milipascals when you enter the cp command. \n"
                                              "-------+--------------------------------------------------- \n"
                                              "       | Saves a linear calibration point for the sensor when you have a pre-measured value.\n"
                                              "cpl    | Command format: cpl;998;1010 \n"
                                              "       | Here 998 was the value that the sensor reported when exposed  \n"
                                              "       | to 1010 milipascals of pressure. \n"
                                              "-------+--------------------------------------------------- \n"
                                              "clear  | Deletes all saved calibration values for this sensor \n"
                                              "-------+--------------------------------------------------- \n"
                                              "q      | Quit / exit pressure sensor mode.\n"
                                              "-------+--------------------------------------------------- \n\n");

  if (String("cp") == latest_command.cmd_name)
  {
    add_calibration_point(pressure_get_raw_reading(), latest_command.value1_number, &pressure_sensor_calibration);
    write_sensor_calibration("pressure", false, &pressure_sensor_calibration);
    print_calibration_values(&pressure_sensor_calibration);
  }
  else if (String("cpl") == latest_command.cmd_name)
  {
    add_calibration_point(latest_command.value1_number, latest_command.value2_number, &pressure_sensor_calibration);
    write_sensor_calibration("pressure", false, &pressure_sensor_calibration);
    print_calibration_values(&pressure_sensor_calibration);
  }
  else if (String("clear") == latest_command.cmd_name)
  {
    clear_sensor_calibration(&pressure_sensor_calibration);
    write_sensor_calibration("pressure", false, &pressure_sensor_calibration);
    print_calibration_values(&pressure_sensor_calibration);
  }
  else
  {
    println(general_cmd_mode_help);
    println(cmd_help_msg);
    println("===== Pressure Sensor Calibration Mode. Send q to exit. =====");
    print_calibration_values(&pressure_sensor_calibration);
    println("=============================================================");
  }
  return false;
}

#endif

