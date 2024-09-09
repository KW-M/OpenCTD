#include "Arduino.h"
// Double include guard
#ifndef COMMAND_MODE_FUNCTIONS_H
#define COMMAND_MODE_FUNCTIONS_H

#include "ssrov_ctd_pinouts_and_constants.hpp"

#define serial_input_timeout_max_count 300 // How many milliseconds to wait for aditional characters before a command is considered complete, Should be plenty as commands are sent in one go.
#define COMMAND_MODE_TIMEOUT 20000         // ms before the command mode should automatically exit - to prevent any unlikely glitch that causes command mode to be entered while at sea from stopping data collection.
#define FULL_CMD_MAX_LENGTH 120            // Number of characters in the longest readable command

struct UserCommand
{
  String full_cmd; // stores the latest serial input command from the computer after read_serial_input_characters has been called enough.
  String cmd_name;                     // the begining part of the command eg "cpe" in "cpe,3.5,ssss"
  String value1_string;                // the first value part of the command eg "3.5" in "cpe,3.5,ssss"
  float value1_number;                // the first value part of the command as a float number eg 3.5 in "cpe,3.5,ssss"
  String value2_string;                // the second value part of the command eg "ssss" in "cpe,3.5,ssss"
  float value2_number;                // // the second value part of the command as a float number eg NAN in "cpe,3.5,ssss"
};

enum SensorTypes
{
  NONE,
  CONDUCTIVITY,
  TEMP,
  PRESS,
  LIGHT,
  ORIENTATION
};

extern SensorTypes command_mode_active_sensor;
extern uint16_t latest_cmd_cursor_index; // keeps track of where in the latest_cmd string to insert the next character.
extern bool cmd_ready;                   // keeps track of whether we have recieved a full command from the users's computer which is ready to get processed;
extern bool debug_mode;

// -----------------------------------------
// ------- Forward Function Definitions (So the compiler knows these functions exist in later includes) --------
// -----------------------------------------

void write_onboard_config();

bool light_sensor_user_command_handler(UserCommand, const __FlashStringHelper *);
void light_sensor_show_live_data();

bool ec_i2c_user_command_handler(UserCommand, const __FlashStringHelper *);
void ec_i2c_show_live_data();

bool temp_sensor_user_command_handler(UserCommand, const __FlashStringHelper *);
void temp_sensor_show_live_data();

bool pressure_sensor_user_command_handler(UserCommand, const __FlashStringHelper *);
void pressure_sensor_show_live_data();

// -----------------------------------------
// ------------- Functions -----------------
// -----------------------------------------

void clear_plot_screen();
void read_serial_input_characters();
String parse_command();
bool handle_user_commands();

#endif
