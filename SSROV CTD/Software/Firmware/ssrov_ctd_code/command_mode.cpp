#include "command_mode.hpp"

// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

char latest_full_cmd[FULL_CMD_MAX_LENGTH];  // stores the latest serial input command from the computer after read_serial_input_characters has been called enough.
uint16_t latest_cmd_cursor_index = 0;       // keeps track of where in the latest_cmd string to insert the next character.
bool cmd_read_fully = true;                // keeps track of whether we have recieved a full cmd form the user's computer yet (or only a few characters);
bool cmd_ready = false;

char *latest_cmd_str;          // stores just the parsed input command part (before the first comma)
char *latest_cmd_str_value_1;  // stores just the parsed input value part 1 (after the first comma)
float latest_cmd_value_1;      // stores just the parsed input value part 1 as a float (after the first comma)
char *latest_cmd_str_value_2;  // stores just the parsed input value part 1 (after the first comma)
float latest_cmd_value_2;      // stores just the parsed input value part 2 (after the first comma)

const __FlashStringHelper *cmd_help_mssg = F("\n\n----- Available Commands ------\n"
                                             //  "debug - toggle debuging mode off/on: debugging mode prints helpful info to the serial monitor while logging.\n" // note debug mode is now always onn
                                             "clock     | Sets the current time.\n"
                                             "          | Command format: clock,Jul 06 2022,19:06:56 \n"
                                             "----------+----------------------------------------------- \n"
                                             "raw       | Toggle logging uncalibrated instead of calibrated sensor values\n"
                                             "          | Make sure this is OFF before deploying!\n"
                                             "----------+----------------------------------------------- \n"
                                             "log_delay | Sets the number of seconds between each new row logged in the output file\n"
                                             "          | NOTE: Sensors will run at their maximum sample rate even if log_delay is faster\n"
                                             "          | Command format: log_delay,0.2 \n"
                                             "----------+----------------------------------------------- \n"
                                             "light     | calibrate the light sensor and view measurments \n"
                                             "----------+----------------------------------------------- \n"
                                             "temp      | calibrate the temperature probes and view measurments \n"
                                             "----------+----------------------------------------------- \n"
                                             "pressure  | calibrate the pressure sensor and view measurments \n"
                                             "----------+----------------------------------------------- \n"
                                             "ec        | electrical conductivity mode: calibrate the sensor and view measurments \n"
                                             "----------+----------------------------------------------- \n"
                                             "wipe      | Format / Erase SD card. Will delete ALL saved data & calibrations! \n"
                                             "          | Backup any data and saved_calibrations folder first!  \n"
                                             "----------+----------------------------------------------- \n"
                                             "q         | quit/exit command mode and resume datalogging.\n"
                                             "----------------------------------------------------------");

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
    if (Serial.available()) inchar = (char)Serial.read();                                                                                                                                  // if a new character is still available, get the new char
    if (inchar == '\n' or inchar == '\r' or latest_cmd_cursor_index == FULL_CMD_MAX_LENGTH - 1 or serial_input_timeout_counter >= serial_input_timeout_max_count) {  // if the char is a return (\n) or line feed (\r) or the string has filled up all available memory for this string, stop,reset the cursor and return true.
      // if the command is done, finish up the string and mark that the command is complete:
      cmd_read_fully = true;
      cmd_ready = true;
      latest_full_cmd[latest_cmd_cursor_index] = '\0';  // add the "null terminator" char to mark the string as finished
      latest_cmd_cursor_index = 0;                      // reset the cursor position back to 0;
    } else if (inchar != 0) {
      // otherwise (if the char isn't the null character, 0) add the new character to the full cmd string:
      latest_full_cmd[latest_cmd_cursor_index] = inchar;  // add the new char to the string;
      latest_cmd_cursor_index++;                          // move the cursor forward;
    }
    serial_input_timeout_counter++;
  }
}

char *parse_command() {
  size_t full_cmd_len = strlen(latest_full_cmd);
  for (size_t i = 0; i <= full_cmd_len; i++) {         // set all char to lower case.
    latest_full_cmd[i] = tolower(latest_full_cmd[i]);  // avoid "Sleep" ≠ "sleep"
  }

  // Extract the first part of the command
  latest_cmd_str = strtok(latest_full_cmd, ",");
  latest_cmd_str_value_1 = strtok(NULL, ",");
  latest_cmd_value_1 = atof(latest_cmd_str_value_1);
  latest_cmd_str_value_2 = strtok(NULL, ",");
  latest_cmd_value_2 = atof(latest_cmd_str_value_2);
  //   Serial.print(latest_full_cmd);
  //     Serial.print( " : " );
  //   Serial.print(latest_cmd_str);
  //   Serial.print( " | " );
  //   Serial.print(latest_cmd_value_1);
  //   Serial.print( " | " );
  //   Serial.print(latest_cmd_value_2);
  //  Serial.print( " = " );
  //   Serial.println(temp_value_str);
  return latest_cmd_str;
}

// Function to clear the arduino serial plotter window by simply writing alot of zeros to the serial console.
void clearPlotScreen() {
  for (uint16_t i = 0; i < 1000; i++) {
    Serial.println("0,0,0,0,0,0,0,0,0,0,0,0");
  }
}

bool process_command() {
  parse_command();

  if (strcmp("q", latest_cmd_str) == 0) {
    return true;
  }

  else if (strcmp_P(latest_cmd_str, "d") == 0) {
    sd_usb_passthrough_clear_read_flag();
    return true;
  }

  else if (strcmp("raw", latest_cmd_str) == 0) {

    onboard_config.log_raw_values = !onboard_config.log_raw_values;
    Serial.print(F("Raw (uncalibrated) logging mode is "));
    if (onboard_config.log_raw_values == true) {
      Serial.println("on - Make sure this is OFF before deploying!");
    } else {
      Serial.println("off");
    }
    write_onboard_config();
    return true;

  } else if (strcmp("clock", latest_cmd_str) == 0) {
    // clock,Jul 06 2022,09:32:50

    latest_cmd_str_value_1[0] = toupper(latest_cmd_str_value_1[0]);  // capitalize the month
    Serial.println(strlen(latest_cmd_str_value_1));
    Serial.println(strlen(latest_cmd_str_value_2));
    if (strlen(latest_cmd_str_value_1) == 11 and strlen(latest_cmd_str_value_2) == 8) {
      DateTime realDate = DateTime(latest_cmd_str_value_1, latest_cmd_str_value_2);
      time_t t = realDate.unixtime();
      Serial.println(ctime(&t));
      rtc.adjust(realDate);
    } else {
      Serial.println("Date/Time not formatted correctly! Make sure to add leading zero to day, hr, minutes and seconds if only one digit.");
    }
  } else if (strcmp("log_delay", latest_cmd_str) == 0) {
    onboard_config.seconds_between_log_events = latest_cmd_value_1;
    write_onboard_config();
    return true;
  }

  else if (strcmp("wipe", latest_cmd_str) == 0) {
    format_sd_card();
    return true;
  }

  else if (strcmp_P(latest_cmd_str, "debug") == 0) {
    //   onboard_config.debugging_mode_on = !onboard_config.debugging_mode_on;
    //   Serial.print(F("Debuging mode is "));
    //   utility_nicely_print_bool(onboard_config.debugging_mode_on);
    //   writeOnboardConfig();
  }

#if ENABLE_PHOTORESISTOR
  else if (strcmp("light", latest_cmd_str) == 0) {
    photoresistor_command_mode_loop();
    return true;
  }
#endif

#if ENABLE_CONDUCTIVITY_SENSOR
  else if (strcmp("ec", latest_cmd_str) == 0) {
    ec_i2c_command_mode_loop();
    return true;
  }
#endif

#if ENABLE_TEMP_PROBE
  else if (strcmp("temp", latest_cmd_str) == 0) {
    temp_command_mode_loop();
    return true;
  }
#endif

#if ENABLE_PRESSURE_SENSOR
  else if (strcmp("pressure", latest_cmd_str) == 0) {
    pressure_command_mode_loop();
    return true;
  }
#endif

  else {
    Serial.println(cmd_help_mssg);
  }

  return false;
}

void command_mode_loop() {

  bool should_exit = process_command();  // process command that triggered the command mode loop
  if (should_exit) return;

  Serial.println(cmd_help_mssg);
  sd_usb_passthrough_disable();  // disable sd passthrough which might interfere with writing the config / calibration files while in command mode.
  unsigned long int command_mode_last_activity_time = millis();
  while (true) {


    power_ctrl_check_mag_switch();

    // if the timeout runs out, exit command mode:
    if (millis() > command_mode_last_activity_time + COMMAND_MODE_TIMEOUT) {
      break;
    }

    // check for new characters from serial monitor (user's computer):
    read_serial_input_characters();
    if (!cmd_ready) continue;

    // handle command:
    command_mode_last_activity_time = millis();
    bool should_exit = process_command();
    if (should_exit) break;

  }
}
