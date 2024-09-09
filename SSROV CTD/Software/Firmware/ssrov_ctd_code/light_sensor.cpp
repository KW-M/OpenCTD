#include "Wire.h"
#include "light_sensor.hpp"
#if ENABLE_LIGHT_SENSOR

#include "utility_functions.hpp"
#include "power_control.hpp"
#include "command_mode.hpp"
#include "config_storage.hpp"
#include "sdcard.hpp"

// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

hp_BH1750 BH1750; //  create the sensor
SensorCalibration_t light_sensor_calibration;
double light_sensor_latest_value = NAN;

// ----------------------------------------
// -------- Light Sensor Functions --------
// ----------------------------------------

bool light_sensor_setup_sensor()
{
  light_sensor_calibration = read_sensor_calibration("light_sensor", true);
  bool avail = BH1750.begin(LIGHT_SENSOR_I2C_ADDR,&Wire); // init the sensor with address pin connetcted to ground
                                                    // result (bool) wil be be "false" if no sensor found
  if (!avail)
  {
    println("No BH1750 light sensor found!");
    return false;
  }
  else
  {
    BH1750.start(BH1750_QUALITY_HIGH2, 254); // starts the first measurement
    return true;
  }
}

float light_sensor_get_full_range_value()
{
  // based on https://github.com/Starmbi/hp_BH1750/wiki/timing-at-saturation
  float val = BH1750.getLux();
  if (BH1750.saturated())
    val = val * BH1750.getMtregTime() / BH1750.getTime(); // additional line for the hack!
  return val;
}

bool light_sensor_refresh_value()
{
  // wait until a measurement has finished
  if (BH1750.hasValue())
  {
    light_sensor_latest_value = light_sensor_get_full_range_value();
    // perform a rough pre-measurement to adjust the expected range of the sensor depending on available light.
    // https://github.com/Starmbi/hp_BH1750/wiki/working-principle#auto-ranging-function
    // BH1750.adjustSettings(0.5, true);
    BH1750.start(BH1750_QUALITY_HIGH2, 254); // starts the next measurement
    return true;
  }
  return false;
}

float light_sensor_get_lux_light_level()
{
  return light_sensor_latest_value;
}

bool light_sensor_is_calibrated()
{
  return light_sensor_calibration.length > 0;
}

float light_sensor_get_calibrated_light_level()
{
  float raw_light_lvl = light_sensor_get_lux_light_level();
  return calculate_calibrated_value(raw_light_lvl, &light_sensor_calibration);
}

float light_sensor_get_value()
{
  if (light_sensor_is_calibrated() and !onboard_config.log_raw_values)
  {
    return light_sensor_get_calibrated_light_level();
  }
  else
  {
    return light_sensor_get_lux_light_level();
  }
}

void light_sensor_show_live_data()
{
  // print the raw measured value and calibrated value if it is available:
  if(light_sensor_refresh_value()) {
    float raw_light_level = light_sensor_get_lux_light_level();
    print_plot_value("Raw_light_level_lux",raw_light_level);
    if (light_sensor_calibration.length > 0)
    {
      float calibrated_light_level = light_sensor_get_calibrated_light_level();
      print_plot_value("Calibrated_light_level_lux",calibrated_light_level);
    }
    println("");
  }
}

bool light_sensor_user_command_handler(UserCommand latest_command, const __FlashStringHelper *general_cmd_mode_help)
{
  const __FlashStringHelper *cmd_help_msg = F("+-- Light Sensor Calibration Mode - Available Commands ---+\n"
                                              "+------+--------------------------------------------------+\n"
                                              "       | Saves a calibration point for the sensor with the current light level.\n"
                                              "cp     | Command format: cp;76 \n"
                                              "       | Here 76 is the known amount of light hitting \n"
                                              "       | the sensor in lux when you enter the cp command. \n"
                                              "-------+-------------------------------------------------- \n"
                                              "       | Saves a linear calibration point for the sensor when you have a pre-measured value.\n"
                                              "cpl    | Command format: cpl;0.1;76 \n"
                                              "       | Here 0.1 was the value that the light sensor reported when exposed to 76 lux. \n"
                                              "-------+-------------------------------------------------- \n"
                                              "clear  | Deletes all saved calibration values for this sensor \n"
                                              "-------+-------------------------------------------------- \n"
                                              "q      | Quit / exit light sensor mode.\n"
                                              "-------+-------------------------------------------------- \n\n");

  // handle
  if (String("cp") == latest_command.cmd_name)
  {
    add_calibration_point(light_sensor_get_lux_light_level(), latest_command.value1_number, &light_sensor_calibration);
    write_sensor_calibration("light_sensor", true, &light_sensor_calibration);
    print_calibration_values(&light_sensor_calibration);
  }
  else if (String("cpl") == latest_command.cmd_name)
  {
    add_calibration_point(latest_command.value1_number, latest_command.value2_number, &light_sensor_calibration);
    write_sensor_calibration("light_sensor", true, &light_sensor_calibration);
    print_calibration_values(&light_sensor_calibration);
  }
  else if (String("clear") == latest_command.cmd_name)
  {
    clear_sensor_calibration(&light_sensor_calibration);
    write_sensor_calibration("light_sensor", true, &light_sensor_calibration);
    print_calibration_values(&light_sensor_calibration);
  }
  else
  {
    println(general_cmd_mode_help);
    println(cmd_help_msg);
    println("====== Light Sensor Calibration Mode. Send q to exit. =======");
    print_calibration_values(&light_sensor_calibration);
    println("=============================================================");
  }
  return false;
}

#endif
