#include "Adafruit_USBD_CDC.h"
#include <cmath>
#include "command_mode.hpp"
#include "indicator_light.hpp"
#include "power_control.hpp"
#include "utility_functions.hpp"
#include "clock.hpp"
#include "config_storage.hpp"
#include "time.h"
#include "sd_usb_passthrough.hpp"
#include "sd_format_card.hpp"

// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

SensorTypes command_mode_active_sensor = SensorTypes::NONE;
UserCommand latest_user_command = {
  String(""),  // full_cmd
  String(""),  // cmd_name
  String(""),  // value1_string
  NAN,         // value1_number
  String(""),  // value2_string
  NAN,         // value2_number
};
uint16_t latest_cmd_cursor_index = 0;  // keeps track of where in the latest_cmd string to insert the next character.
bool cmd_read_fully = true;            // keeps track of whether we have recieved a full cmd form the user's computer yet (or only a few characters);
bool cmd_ready = false;
bool debug_mode = false;

const __FlashStringHelper *cmd_general_help_msg = F(
  "\n"
  "\n"
  "+---------+---------- Available Commands -----------------+ \n"
  "light     | Calibrate the light sensor and view measurements \n"
  "----------+------------------------------------------------ \n"
  "temp      | Calibrate the temperature probes and view measurements \n"
  "----------+------------------------------------------------ \n"
  "pressure  | Calibrate the pressure sensor and view measurements \n"
  "----------+------------------------------------------------ \n"
  "ec        | Calibrate the conductivity sensor and view measurements \n"
  "----------+------------------------------------------------ \n"
  //  "debug - toggle debuging mode off/on: debugging mode prints helpful info to the serial monitor while logging.\n" // note debug mode is now always onn
  "clock     | Sets the current time.\n"
  "          | Command format: clock;Jul 06 2022;19: 06: 56 \n"
  "----------+--+--------------------------------------------- \n"
  "log_interval | Sets the delay (in seconds) between rows in the csv log \n"
  "             | NOTE: Each sensors will update up to its maximum \n"
  "             | sample rate even if log_interval is faster \n"
  "             | Command format: log_interval;0.5 \n"
  "----------+--+--------------------------------------------- \n"
  "raw       | Toggle logging uncalibrated instead of calibrated sensor values\n"
  "          | Make sure this is OFF before deploying!\n"
  "----------+------------------------------------------------ \n"
  "wipe      | Format the SD card. Will DELETE ALL saved data, calibrations, & files! \n"
  "          | Backup any data and saved_calibrations folder first if you need them!  \n"
  "----------+------------------------------------------------ \n"
  "q         | quit/exit command mode and re-enable usb passthrough.\n"
  "+---------+-----------------------------------------------+");

// -----------------------------------------
// ------------- Functions -----------------
// -----------------------------------------

uint16_t serial_input_timeout_counter = 0;  // counter to keep track of the full message recived timeout
void read_serial_input_characters() {
  cmd_ready = false;
  if (Serial.available()) {            // if a new character is available
    serial_input_timeout_counter = 0;  // reset the timeout counter since we have a new avalilable char (ie: the start of a new cmd).
    cmd_read_fully = false;
  }
  while (cmd_read_fully == false) {
    char inchar = 0;  // set a char placeholder for the next recived charter as the null character: askii character code = 0
    if (Serial.available())
      inchar = (char)Serial.read();                                                                                                                                  // if a new character is still available, get the new char
    if (inchar == '\n' or inchar == '\r' or latest_cmd_cursor_index == FULL_CMD_MAX_LENGTH - 1 or serial_input_timeout_counter >= serial_input_timeout_max_count) {  // if the char is a return (\n) or line feed (\r) or the string has filled up all available memory for this string, stop,reset the cursor and return true.
      // if the command is done, finish up the string and mark that the command is complete:
      cmd_read_fully = true;
      cmd_ready = true;
      latest_cmd_cursor_index = 0;  // reset the cursor position back to 0;
    } else if (inchar != 0) {
      // otherwise (if the char isn't the null character, 0) add the new character to the full cmd string:
      if (latest_cmd_cursor_index == 0) latest_user_command.full_cmd = "";
      latest_user_command.full_cmd += inchar;  // add the new char to the string;
      latest_cmd_cursor_index++;               // move the cursor forward;
    }
    serial_input_timeout_counter++;
  }
}

String parse_command() {
  size_t full_cmd_len = latest_user_command.full_cmd.length();
  latest_user_command.full_cmd.toLowerCase();
  // for (size_t i = 0; i <= full_cmd_len; i++)
  // {                                     // set all char to lower case.
  //   temp_cmd[i] = tolower(temp_cmd[i]); // avoid "Sleep" ≠ "sleep"
  // }

  // latest_user_command.value1_string = ;
  // latest_user_command.value2_string = emptystring;
  latest_user_command.value1_number = NAN;
  latest_user_command.value2_number = NAN;


  // Extract the first part of the command
  int delimiter1 = latest_user_command.full_cmd.indexOf(";");
  int delimiter2 = latest_user_command.full_cmd.indexOf(";", delimiter1 + 1);
  Serial.print(delimiter1);  Serial.print(",");
  Serial.println(delimiter2);
  latest_user_command.cmd_name = latest_user_command.full_cmd.substring(0, delimiter1);
  if (delimiter1 > 0) {
    latest_user_command.value1_string = latest_user_command.full_cmd.substring(delimiter1 + 1, delimiter2);
  } else {
    latest_user_command.value1_string = "";
  }
  if (delimiter2 > 0) {
    latest_user_command.value2_string = latest_user_command.full_cmd.substring(delimiter2 + 1, full_cmd_len);
  } else {
    latest_user_command.value2_string = "";
  }


  // Extract the second number part of the command
  if (latest_user_command.value1_string.length() != 0) {
    latest_user_command.value1_number = atof(latest_user_command.value1_string.c_str());
  }

  // Extract the thrid number part of the command
  if (latest_user_command.value2_string.length() != 0) {
    latest_user_command.value1_number = atof(latest_user_command.value2_string.c_str());
  }

  Serial.println(latest_user_command.full_cmd);
  Serial.println(latest_user_command.cmd_name);
  Serial.println(latest_user_command.value1_number * 100);
  Serial.println(latest_user_command.value1_number * 100);

  return latest_user_command.cmd_name;
}

// Function to clear the arduino serial plotter window by writing a-lot of zeros to the serial console.
void clear_plot_screen() {
  for (uint16_t i = 0; i < 200; i++)
    Serial.println("0");
}

bool process_command() {
  parse_command();

  // q: Quit/Exit
  if (String("q") == latest_user_command.cmd_name) {
    command_mode_active_sensor = SensorTypes::NONE;
    return true;
  }

  // d: Disable USB Passthrough (until restart)
  if (String("d") == latest_user_command.cmd_name) {
    sd_usb_passthrough_clear_read_flag();
    return true;
  }

  // raw: Toggle raw mode - ignores all calibrations & saves raw sensor measurements to SD Card instead.
  if (String("raw") == latest_user_command.cmd_name) {
    onboard_config.log_raw_values = !onboard_config.log_raw_values;
    print(F("Raw (uncalibrated) logging mode is "));
    if (onboard_config.log_raw_values == true) {
      println("on - Make sure this is OFF before deploying!");
    } else {
      println("off");
    }
    write_onboard_config();
    return true;
  }

  // clock: Get or set the current clock date & time
  // - command format: clock,Jul 06 2022,09:32:50
  if (String("clock") == latest_user_command.cmd_name) {
    if (latest_user_command.value1_string.length() == 0) {
      // Print the current time if no value is provided
      clock_print_time();
      println("To set the time, use the command format: clock;Jul 06 2022;09:32:50");
    } else {
      // parse the given date & time and apply it to the RTC clock adjustment.
      latest_user_command.value1_string[0] = toupper(latest_user_command.value1_string[0]);  // capitalize the month
      Serial.println(latest_user_command.value1_string.length());
      Serial.println(latest_user_command.value2_string.length());
      if (latest_user_command.value1_string.length() == 11 and latest_user_command.value2_string.length() == 8) {
        DateTime realDate = DateTime(latest_user_command.value1_string.c_str(), latest_user_command.value2_string.c_str());
        time_t t = realDate.unixtime();
        println(ctime(&t));
        rtc.adjust(realDate);
      } else {
        println("Date/Time not formatted correctly! Make sure to add leading zero to day, hour, minutes and seconds if any are only one digit.");
      }
    }
    return true;
  }

  // log_interval: set the number of seconds between saved log entries on the SD Card
  // - command format: log_interval,0.8
  if (String("log_interval") == latest_user_command.cmd_name) {
    onboard_config.seconds_between_log_events = latest_user_command.value1_number;
    write_onboard_config();
    return true;
  }

  // wipe: erase and format the SD Card using the SDFat Library tool
  if (String("wipe") == latest_user_command.cmd_name) {
    format_sd_card();
    println("Please turn the CTD Off & On");
    return true;
  }

  // debug: Toggle extra logging and Serial print statements functionality like Battery charge and Avialable Memory
  else if (String("debug") == latest_user_command.cmd_name) {
    debug_mode = !debug_mode;
    // onboard_config.debugging_mode_on = debug_mode;
    print(F("Debuging mode is "));
    utility_nicely_print_bool(debug_mode);
    // write_onboard_config();
    return true;
  }

  // light: Switch to light sensor live viewing/calibrating command mode
#if ENABLE_LIGHT_SENSOR
  if (String("light") == latest_user_command.cmd_name) {
    clear_plot_screen();
    command_mode_active_sensor = SensorTypes::LIGHT;
  }
#endif

  // ec: Switch to conductivity sensor live viewing/calibrating command mode
#if ENABLE_CONDUCTIVITY_SENSOR
  if (String("ec") == latest_user_command.cmd_name) {
    clear_plot_screen();
    command_mode_active_sensor = SensorTypes::CONDUCTIVITY;
  }
#endif

  // temp: Switch to temperature sensor live viewing/calibrating command mode
#if ENABLE_TEMP_PROBE
  if (String("temp") == latest_user_command.cmd_name) {
    clear_plot_screen();
    command_mode_active_sensor = SensorTypes::TEMP;
  }
#endif

  // pressure: Switch to pressure sensor live viewing/calibrating command mode
#if ENABLE_PRESSURE_SENSOR
  if (String("pressure") == latest_user_command.cmd_name) {
    clear_plot_screen();
    command_mode_active_sensor = SensorTypes::PRESS;
  }
#endif

  // -------------------------------------------------

#if ENABLE_LIGHT_SENSOR
  if (command_mode_active_sensor == SensorTypes::LIGHT) {
    return light_sensor_user_command_handler(latest_user_command, cmd_general_help_msg);
  }
#endif
#if ENABLE_CONDUCTIVITY_SENSOR
  if (command_mode_active_sensor == SensorTypes::CONDUCTIVITY) {
    return ec_i2c_user_command_handler(latest_user_command, cmd_general_help_msg);
  }
#endif
#if ENABLE_TEMP_PROBE
  if (command_mode_active_sensor == SensorTypes::TEMP) {
    return temp_sensor_user_command_handler(latest_user_command, cmd_general_help_msg);
  }
#endif
#if ENABLE_PRESSURE_SENSOR
  if (command_mode_active_sensor == SensorTypes::PRESS) {
    return pressure_sensor_user_command_handler(latest_user_command, cmd_general_help_msg);
  }
#endif

  // -------------------------------------------------

  println(cmd_general_help_msg);
  return true;
}

// void command_mode_loop() {

//   // bool should_exit =
//   // if (should_exit)
//   //   return;

//   // || millis() > command_mode_last_activity_time + COMMAND_MODE_TIMEOUT
//   // unsigned long int command_mode_last_activity_time = millis();
//   // while (true) {

//   //   // power_ctrl_check_switch();;

//   //   // if the timeout runs out, exit command mode:

//   //     break;
//   //   }

//   //   if (!cmd_ready && command_mode_active_sensor == SensorTypes::NONE)
//   //     continue;

//   //   // handle command:
//   //   command_mode_last_activity_time = millis();
//   //   bool should_exit = process_command();
//   //   if (should_exit)
//   //     break;
//   // }
// }

bool handle_user_commands() {

  // Exit early if no computer is connected by USB cable.
  if (!usb_is_connected())
    return false;

  // check for new characters from serial monitor (user's computer):
  read_serial_input_characters();

  // if a full command is ready to be processed
  if (cmd_ready) {
    sd_usb_passthrough_disable();  // disable sd passthrough which might interfere with writing the config / calibration files while in command mode.
    process_command();             // process command that triggered the command mode loop
    sd_usb_passthrough_enable();   // Re-Enable SD USB Passthrough
    return true;
  }

#if ENABLE_LIGHT_SENSOR
  if (command_mode_active_sensor == SensorTypes::LIGHT) {
    light_sensor_show_live_data();
    return true;
  }
#endif

#if ENABLE_CONDUCTIVITY_SENSOR
  if (command_mode_active_sensor == SensorTypes::CONDUCTIVITY) {
    ec_i2c_show_live_data();
    return true;
  }
#endif

#if ENABLE_TEMP_PROBE
  if (command_mode_active_sensor == SensorTypes::TEMP) {
    temp_sensor_show_live_data();
    return true;
  }
#endif

#if ENABLE_PRESSURE_SENSOR
  if (command_mode_active_sensor == SensorTypes::PRESS) {
    pressure_sensor_show_live_data();
    return true;
  }
#endif

  return false;
}
