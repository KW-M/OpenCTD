// Double include guard
#ifndef COMMAND_MODE_FUNCTIONS_H
#define COMMAND_MODE_FUNCTIONS_H

#include "ssrov_ctd_pinouts_and_constants.hpp"
#include "power_control.hpp"
#include "utility_functions.hpp"
#include "clock.hpp"
#include "config_storage.hpp"
#include "time.h"
#include "sd_usb_passthrough.hpp"
#include "sd_format_card.hpp"

#define serial_input_timeout_max_count 300        // How many milliseconds to wait for aditional characters before a command is considered complete, Should be plenty as commands are sent in one go.
#define COMMAND_MODE_TIMEOUT 20000                // ms before the command mode should automatically exit - to prevent any unlikely glitch that causes command mode to be entered while at sea from stopping data collection.
#define FULL_CMD_MAX_LENGTH 120                   // Number of characters in the longest readable command
extern char latest_full_cmd[FULL_CMD_MAX_LENGTH]; // stores the latest serial input command from the computer after read_serial_input_characters has been called enough.
extern uint16_t latest_cmd_cursor_index;          // keeps track of where in the latest_cmd string to insert the next character.
extern bool cmd_ready;                            // keeps track of whether we have recieved a full command from the users's computer which is ready to get processed;

extern char *latest_cmd_str;     // stores just the parsed input command part (before the first comma)
extern float latest_cmd_value_1; // stores just the parsed input value part 1 (after the first comma)
extern float latest_cmd_value_2; // stores just the parsed input value part 2 (after the first comma)

// -----------------------------------------
// ------- Forward Function Definitions (So the compiler knows these functions exist in later includes) --------
// -----------------------------------------

void write_onboard_config();
void photoresistor_command_mode_loop();
void ec_i2c_command_mode_loop();
void temp_command_mode_loop();
void pressure_command_mode_loop();

// -----------------------------------------
// ------------- Functions -----------------
// -----------------------------------------

void read_serial_input_characters();
char *parse_command();
void clearPlotScreen();
void command_mode_loop();

#endif
