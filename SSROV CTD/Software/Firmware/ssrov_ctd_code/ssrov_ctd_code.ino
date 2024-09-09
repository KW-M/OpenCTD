// -- Set pins, global constants & #include statements in this file --
// -- (must be included before everything else) ----------------------
#include "ssrov_ctd_pinouts_and_constants.hpp"
// -------------------------------------------------------------------
#include "power_control.hpp"
#include "sd_usb_passthrough.hpp"
#include "utility_functions.hpp"
#include "command_mode.hpp"
#include "indicator_light.hpp"
#include "battery.hpp"
#include "clock.hpp"
#include "sdcard.hpp"
#include "config_storage.hpp"
#include "temp_probe.hpp"
#include "conductivity_sensor.hpp"
#include "light_sensor.hpp"
#include "pressure_sensor.hpp"
#include "orientation_sensor.hpp"

/*  Each tab at the top, and the include statments abvoe corresponds to a file in this folder and those files help organize the different functions.
    We're using .hpp files instead of .ino (though they both just contain code) beacause we can include them in the order we want and they'll all get compiled
    as if all the code in each .hpp file replaced the corresponding #include line. If we use multiple .INO files, the arduino IDE tries to be helpful and compliles them
    in alphabetical order, but sometimes messes up and then the code breaks. If you want to organize the code in a new file, click the dropdown and select new tab,
    but MAKE SURE to ADD ".hpp" at the end and then ADD ANOTHER #include LINE like the ones above for the new tab.
*/

/* You'll need these libraries:
   [cmd/ctl + click the links in the Arduino IDE to open the library manager - or search for them yourself]:
  - http://librarymanager/All#SdFat_Bill_Greiman < Use "SdFat by Bill Greiman" version 2.2.2 do NOT use the Adafruit fork - it was not as updated at time of writing
  - http://librarymanager/All#RTClib_Adafruit < use "RTClib by Adafruit" version 2.1.3
  - http://librarymanager/All#Adafruit_BNO055 < Use "Adafruit BNO055 by Adafruit" version 1.6.0
  - http://librarymanager/All#SparkFun_MS5803-14BA < Use "SparkFun MS5803-14BA Pressure Sensor by Sparkful" version 1.1.4
  - http://librarymanager/All#DallasTemperature_Miles_Burton < Use "DallasTemperature by Miles Burton" version 3.9.0
  - http://librarymanager/All#hp_BH1750 < use "hp_BH1750 by Stefan Armborst" aka Starmbi on github version 1.0.2
  - http://librarymanager/All#ArduinoJson_Benoit_Blanchon < use "ArduinoJson by Benoit Blanchon" version 6.21.2
  - http://librarymanager/All#MultiMap_Rob_Tillaart < use ""
  - http://librarymanager/All#StreamUtils_Benoit_Blanchon

  Alternative places to install libraries manually:
   [ See this tutorial for how to manually install libraries: https://www.baldengineer.com/installing-arduino-library-from-github.html]
  - https://github.com/millerlp/MS5803_14
  - https://github.com/milesburton/Arduino-Temperature-Control-Library
  - (no longer used) https://github.com/OceanographyforEveryone/OpenCTD/tree/master/OpenCTD_Feather_Adalogger/Support_Code/SoftwareSerial
  - https://github.com/Starmbi/hp_BH1750
*/

// Header row for CSV datalog files (this string gets assembled in the setup() function)
String csv_header = String("");

/* Initital function that runs */
void setup()
{


  // power switch control setup enables the hold to power on/off function.
  power_ctrl_setup();

  // SD Card usb passthrough "pre" setup MUST go before Serial or SD Card setup/initilization and should happen
  // almost immediately after startup for the computer to correctly recognizes the feather as a "Mass Storage Device"
  sd_usb_passthrough_pre_setup();

   // Show board has powered on.
  indicator_light_on(LED_STAT2);

// electrical conductivity pre setup MUST GO BEFORE ANY I2C SETUP - Before clock or other sensor setup (because it temporarlly uses the I2C pins as UART pins)
#if ENABLE_CONDUCTIVITY_SENSOR
  ec_sensor_pre_setup();
#endif

  // SETUP SERIAL CONNECTION ----- (To send text to / recive text from the serial monitor on a computer) -----
  Serial.begin(115200);                                               // Switch to the faster serial baud rate of 115200 - NOTE: On feather M0 boards the baud rate is generally irrelevant/set by the USB host;
  while ((not Serial or Serial.available() == 0) and millis() < 3000) // Wait for computer to connect or a 3 second timeout to elapse before continuing.
  {
    power_ctrl_check_switch();
    indicator_light_flash(LED_STAT1); // flash the red led
    handle_user_commands();
    delay(10);
  }

  #if defined(WIRE_HAS_TIMEOUT)
    Serial.println("Setting i2c timeout");
    Wire.setWireTimeout(25000 /* us */, true /* reset_on_timeout */);
  #endif

  // SETUP CLOCK --------------------------
  print("Initilizing clock - ");
  while (not clock_setup_rtc())
  { // if clock setup fails, the clock_setup_rtc() function will return false and this loop will run, pulsing the indicator light & trying again.
    power_ctrl_check_switch();
    indicator_light_pulse(LED_STAT1);
    handle_user_commands();
    delay(10);
  }
  clock_print_time();
  csv_header += RTC_CSV_HEADER;

  // SETUP SD-CARD -------------------------
  println(F("Initilizing SD Card"));
  while (not sd_setup_sd_card())
  { // NOTE: If the sd card is not inserted or failing this function returns false and so will loop.

    power_ctrl_check_switch();
    indicator_light_flash(LED_STAT1);
    indicator_light_flash(LED_STAT1);
    handle_user_commands();
    delay(1000);
  }
  sd_enable_file_timestamps();
  setup_config_storage();
  write_onboard_config();
  sd_usb_passthrough_post_setup(); // continue Initilizing sd card usb passthrough once sd card is setup.

#if ENABLE_PRESSURE_SENSOR
  println(F("Initilizing pressure sensor "));
  pressure_setup_sensor();
  csv_header += PRESSURE_SENSOR_CSV_HEADER;
#endif

#if ENABLE_TEMP_PROBE
  println(F("Initilizing temperature probes"));
  temp_setup_sensors(); // setup all 3 temperature probes
  csv_header += TEMP_PROBE_CSV_HEADER;
#endif

#if ENABLE_CONDUCTIVITY_SENSOR
  println(F("Initilizing conductivity sensor"));
  ec_setup_sensor(); // setup onboard ec chip
  csv_header += CONDUCTIVITY_SENSOR_CSV_HEADER;
#endif

#if ENABLE_LIGHT_SENSOR
  while (true)
  {
    println(F("Initilizing light sensor..."));
    if (light_sensor_setup_sensor())
      break;
    indicator_light_flash(LED_STAT2);
    handle_user_commands();
    delay(1000);
    power_ctrl_check_switch();
  }
  csv_header += LIGHT_SENSOR_CSV_HEADER;
#endif

#if ENABLE_BNO055_ORIENTATION_SENSOR
  println(F("Initilizing orientation sensor"));
  orient_setup_sensor();
  csv_header += BNO055_ORIENTATION_SENSOR_CSV_HEADER;
#endif

#if ENABLE_BATTERY_MONITOR
  csv_header += BATTERY_MONITOR_CSV_HEADER;
#endif

  println(F("==========================================================================="));
  println(F("To see available commands - send any letter from the Arduino Serial Monitor."));
  println(F("==========================================================================="));
  Serial.flush();
  indicator_light_off(LED_STAT2);
}

unsigned long lastDatalogTime = 0;

void loop()
{
  power_ctrl_check_switch();
  if (board_will_power_down)
  {
    // Turn OFF everything to indicate
    // power down state
    sd_usb_passthrough_disable();
    sd_close();
    indicator_light_off(LED_STAT1);
    indicator_light_off(LED_STAT2);
    indicator_light_off(LED_STAT3);
    return;
  }

  handle_user_commands();
  if (usb_is_connected())
  {
    indicator_light_on(LED_STAT1);
    if (!debug_mode)
      return;
  }
  else
  {
    command_mode_active_sensor = SensorTypes::NONE;
    indicator_light_off(LED_STAT1);
  }

  // if (sd_usb_passthrough_read_flag_is_set()) {2345
  //   // if (user_command_has_been_processed)
  //   delay(1000);
  //   println(F("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nSD Card Passthrough Active - Send the letter d to disable and start logging to the sd card. \nSend any other letter to see available commands"));
  //   return;  // do not continue logging while passthrough is active
  // }

#if ENABLE_LIGHT_SENSOR
  light_sensor_refresh_value();
#endif

#if ENABLE_BNO055_ORIENTATION_SENSOR
  // check if the orentation sensor hasn't locked on
  if (not orient_sensor_locked_on())
  {
    print(F("\nCompass not locked on. Rotate the board until light stops flashing. "));
    orient_displayCalibrationStatus();
    indicator_light_flash(LED_STAT1);
  }
#endif

  // don't continue if not enough time has elapsed between the last log event.
  if (millis() < lastDatalogTime + (onboard_config.seconds_between_log_events * 1000))
    return;
  lastDatalogTime = millis();

  // Check if the data log file is working by trying to write the new line character to the datalog:
  // This will be true (ie: not working) on the first loop because the datalogFile isn't open yet, so we open & setup the log file inside the if statement below.

  while (!datalogFile or !sd_log_newline(datalogFile))
  {
    println(F("\nSetting Up Datalog File..."));
    if (csv_header.endsWith(","))
      csv_header.substring(0, csv_header.length() - 1); // remove trailing comma from the csv header
    setup_datalog_file(csv_header.c_str());
    write_onboard_config();
    indicator_light_pulse(LED_STAT1);
  }

  // show a warning if we are set to log RAW / uncalibrated values:
  if (onboard_config.log_raw_values && usb_is_connected())
    print(F("[SHOWING_UNCALIBRATED_VALUES!]"));

  String time = clock_get_datetime_string();
  log_string("Date_Time", time.c_str()); // log the date & time from the real time clock and print it to the console

#if ENABLE_PRESSURE_SENSOR
  log_value("Pressure_mBar", pressure_get_value()); // log the pressure sensor value
#endif

#if ENABLE_TEMP_PROBE
  temp_probes_refresh_values();            // ask all the temperature probes to take a new measurement (if they are ready).
  log_value("Temp1_C", temp_get_value(0)); // write the last measured value of the 1st temp probe to the console
  log_value("Temp2_C", temp_get_value(1)); // write the last measured value of the 2nd temp probe to the console
  log_value("Temp3_C", temp_get_value(2)); // write the last measured value of the 3rd temp probe to the console
#endif

#if ENABLE_CONDUCTIVITY_SENSOR
  log_value("Conductivity", ec_get_value());
#endif

#if ENABLE_LIGHT_SENSOR
  log_value("LightLvl_lux", light_sensor_get_value());
#endif

#if ENABLE_BNO055_ORIENTATION_SENSOR
  orient_update_values(); // Ask the orientation sensor/IMU to measure the acceleration/orientation etc. at this point in time.
  log_value("cumulative_yaw_angle", orient_sensor_has_locked_on ? orient_get_cumulative_yaw_angle() : NAN);
  log_value("NS_pitch", orient_sensor_has_locked_on ? orient_get_NS_pitch() : NAN);
  log_value("EW_pitch", orient_sensor_has_locked_on ? orient_get_EW_pitch() : NAN);
  log_value("NS_accel", orient_sensor_has_locked_on ? orient_get_NS_accel() : NAN);
  log_value("EW_accel", orient_sensor_has_locked_on ? orient_get_EW_accel() : NAN);
#endif

#if ENABLE_BATTERY_MONITOR
  log_value("Battery", battery_get_value());
#endif

  print("Mem:");
  Serial.print(available_memory());
  print(",");

  log_value("Memory", available_memory());

  indicator_light_flash(LED_STAT2);
}

void log_value(const char *name, float value)
{
  return log_value(name, value, false);
}

void log_value(const char *name, float value, bool isRaw)
{
  if (usb_is_connected())
  {
    if (Serial.availableForWrite() == 0)
      return;
    if (isRaw)
      print("RAW_");
    Serial.print(name);
    print("(");
    Serial.print(value, 5);
    print("):");
    Serial.print(value);
    print(",");
  }
  else
  {
    sd_log_value(datalogFile, value);
  }
}

void log_string(const char *name, const char *str)
{
  if (usb_is_connected())
  {
    if (Serial.availableForWrite() == 0)
      return;
    Serial.print(name);
    Serial.print(str);
    print(",");
  }
  else
  {
    sd_log_string(datalogFile, str);
  }
}
