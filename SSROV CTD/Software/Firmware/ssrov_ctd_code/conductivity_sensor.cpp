#include <sys/_types.h>
#include <cctype>
#include <cstring>
#include "conductivity_sensor.hpp"
#if ENABLE_CONDUCTIVITY_SENSOR

#include "utility_functions.hpp"
#include "power_control.hpp"
#include "sdcard.hpp"
#include "command_mode.hpp"
#include "src/Ezo_I2c_lib/Ezo_i2c.h"
#include "src/SoftwareSerial/SoftwareSerial.h"
// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

Ezo_board *ecChip = NULL;
Ezo_board::errors ecLastError = Ezo_board::errors::SUCCESS;

// ----------------------------------------------------
// ----------- Conductivity Sensor Functions ----------
// ----------------------------------------------------

void ec_switch_from_uart_to_i2c_mode()
{
  println("Attempting to make conductivity circuit use the I2C protocol... ");

  // Define the (temporary) SoftwareSerial port for conductivity.
  SoftwareSerial *ecSerial = new SoftwareSerial(EC_RX_PIN_NUM, EC_TX_PIN_NUM);
  ecSerial->begin(9600); // Set baud rate for conductivity circuit.
  delay(100);

  // send the I2C mode command formatted like this: I2C,100\r where 100 is the i2c address that the ec chip will use and \r is a carrage return character.
  ecSerial->print("I2C,");
  ecSerial->print(EC_I2C_ADDR);
  ecSerial->write('\r');
  delay(100);

  // stop & clean up the software Serial port.
  ecSerial->end();
  // delete ecSerial;
}

void ec_sensor_pre_setup()
{
  ec_switch_from_uart_to_i2c_mode(); // a little overkill, but this is here because it will set the ec board to use i2c protocol to catch the cases that the ec chip was reset (never happened to me) or was never set up to use i2c mode in the first place, and is using uart mode instead.
}

void ec_setup_sensor()
{
  ecChip = new Ezo_board(EC_I2C_ADDR);
}

bool sentStatusCheckRequest = false;
bool ec_i2c_log_if_error()
{
  ecLastError = ecChip->get_error();
  switch (ecLastError)
  {                                // switch case based on what the last response code was.
  case Ezo_board::errors::SUCCESS: // decimal 1 means the command was successful.
    return false;

  case Ezo_board::errors::FAIL: // decimal 2 means the command has failed.
    print("EC_Error_Command_Failed");
    return true;                     // exits the switch case.
  case Ezo_board::errors::NOT_READY: // decimal 254 means the command has not yet been finished calculating.
    println("EC_Error_Command_Pending");
    return true;                                // exits the switch case.
  case Ezo_board::errors::NO_DATA:              // decimal 255.
    println("EC_Error_No_Further_Data"); // means there is no further data to send.
    return true;                                // exits the switch case.
  case Ezo_board::errors::NOT_READ_CMD:         //
    println("EC_Error_NOT_READ_CMD");    //
    return true;                                // exits the switch case.
  default:
    // sentStatusCheckRequest = true;
    return false;
  }
}

float ec_i2c_get_measurement()
{
  if (sentStatusCheckRequest == true)
  {
  }
  else if (ecChip->is_read_poll() == true)
  {
    ecLastError = ecChip->receive_read_cmd();
    // check if the last command was
    // ec_i2c_log_if_error();
  }
  if (ecLastError != Ezo_board::errors::NOT_READY)
  {
    ecChip->send_read_cmd();
  }
  return ecChip->get_last_received_reading();
}

float ec_get_value()
{
  return ec_i2c_get_measurement();
}

void ec_i2c_show_live_data()
{
  // every second print sensor measured value (in this case calibrated is our only option)
  if (millis() % 1000 == 0)
  {
    print_plot_value("Conductivity_µS/cm",ec_i2c_get_measurement());
    println("");
  }
}

bool ec_i2c_send_command(const char *cmd, bool quiet = false)
{
  /// - / -----------------------
  /// CODE BELOW ADAPTED FROM: https://files.atlas-scientific.com/Ardunio-I2C-EC-sample-code.pdf
  /// - / -----------------------

  char ec_data[32]; // we make a 32 byte character array to hold incoming data from the EC circuit.
  int time_ = 570;  // used to change the delay needed depending on the command sent to the EZO Class EC Circuit.

  if (cmd[0] == 'c' or cmd[0] == 'r')
    time_ = 570; // if a command has been sent to calibrate or take a reading we wait 570ms so that the circuit has time to take the reading.
  else
    time_ = 250; // if any other command has been sent we wait only 250ms.

  unsigned long reciveTimeoutStart = millis();
  while (ecChip->get_error() != Ezo_board::errors::NO_DATA && millis() - reciveTimeoutStart < 1000) {
    ec_i2c_log_if_error();
    ecChip->receive_cmd(ec_data, 32);
  }
  
  ecChip->send_cmd(cmd);
  if (strcmp(cmd, "sleep") == 0)
  {
    if (!quiet) println("Sleeping...");
    return true; // if it is the sleep command or quit command, we return (to exit the loop) and do nothing. Issuing a sleep command and then requesting data will wake the EC circuit.
  }

  if (!quiet) println("Waiting for response...");
  delay(time_); // wait the correct amount of time for the circuit to complete its instruction.

  ecChip->receive_cmd(ec_data, 32);
  ecLastError = ecChip->get_error();
  // Wire.requestFrom(EC_I2C_ADDR, 32, 1);         // call the circuit and request 32 bytes (this could be too small, but it is the max i2c buffer size for an Arduino)
  // ecLastError = (Ezo_board::errors)Wire.read(); // the first byte is the response code, we read this separately.
  Serial.print(int(ecLastError));

  println(ec_data); // print the response data.
  if (ec_i2c_log_if_error())
    return false;   // log any error that might have occured.
  return true;
}

bool ec_i2c_user_command_handler(UserCommand latest_command, const __FlashStringHelper *general_cmd_mode_help)
{
  const __FlashStringHelper *cmd_help_msg = F("+- Conductivity Sensor Calibration - Available Commands --+\n"
                                              "+-------+--------------------------------------------------+\n"
                                              "cal;dry | Saves calibration for the sensor before wet.\n"
                                              "        | Do this FIRST! \n"
                                              "--------+-------------------------------------------------- \n"
                                              "cal;n   | single point calibration. Where n = any value\n"
                                              "--------+-----+-------------------------------------------- \n"
                                              "Cal;low;12880 | low end 2-point calibration. Where n = any value\n"
                                              "--------+-----+-------------------------------------------- \n"
                                              "Cal;high;12880 | high end 2-point calibration. Where n = any value\n"
                                              "--------------+-------------------------------------------- \n"
                                              "Cal;clear     | Deletes all saved calibration values for this sensor \n"
                                              "-------+-------------------------------------------------- \n"
                                              "q      | quit / exit this mode.\n"
                                              "-------+-------------------------------------------------- \n"
                                              " ...   | see: https://atlas-scientific.com/files/EC_EZO_Datasheet.pdf \n"
                                              "-------+-------------------------------------------------- \n\n");

  // handle the command:
  String cmd_string = latest_command.full_cmd;
  cmd_string.replace(";", ",");
  const char *cmd = cmd_string.c_str();
  print("Received: ");
  println(cmd);
  if (strcmp(cmd, "ec") != 0)
  {
    bool success = ec_i2c_send_command(cmd);
    if (success)
      return true;
  }

  println(general_cmd_mode_help);
  println(cmd_help_msg);
  println("=== Conductivity Sensor Calibration Mode. Send q to exit. ===");
  print("Calibration status: ");
  ec_i2c_send_command("Cal,?", true);
  println("=============================================================");

  return false;
}

// void ec_uart_command_terminal_loop() {
//     // Define the (temporary) SoftwareSerial port for conductivity.
//   SoftwareSerial *ecSerial = new SoftwareSerial(EC_RX_PIN_NUM, EC_TX_PIN_NUM);
//   ecSerial->begin(9600); // Set baud rate for conductivity circuit.
//   delay (100);
//
//   while (true) {
//     if (ecSerial->available()) {
//       char inchar = (char)ecSerial->read();              //get the char we just received
//       if (inchar == '\r') Serial.write('\n');
//       else Serial.write(inchar);
//     }
//     if (Serial.available()) {
//       char outchar = (char)Serial.read();              //get the char we just typed
//       if (outchar == '\n') ecSerial->write('\r');
//       else if (outchar == 'q') break; // exit the loop when 'q' is typed
//       else ecSerial->write(outchar);
//     }
//   }
//
//   delete ecSerial;
// }

#endif
