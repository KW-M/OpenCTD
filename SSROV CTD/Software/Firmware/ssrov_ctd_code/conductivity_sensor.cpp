#include "conductivity_sensor.hpp"
#if ENABLE_CONDUCTIVITY_SENSOR

// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

Ezo_board *ecChip = NULL;
Ezo_board::errors ecLastError = Ezo_board::errors::SUCCESS;

// -----------------------------------------
// ------------- Functions -----------------
// -----------------------------------------

void ec_switch_from_uart_to_i2c_mode() {
  Serial.println("Attempting to make conductivity circuit use the I2C protocol... ");

  // Define the (temporary) SoftwareSerial port for conductivity.
  SoftwareSerial *ecSerial = new SoftwareSerial(EC_RX_PIN_NUM, EC_TX_PIN_NUM);
  ecSerial->begin(9600);  // Set baud rate for conductivity circuit.
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

void ec_sensor_pre_setup() {
  ec_switch_from_uart_to_i2c_mode();  // a little overkill, but this is here because it will set the ec board to use i2c protocol to catch the cases that the ec chip was reset (never happened to me) or was never set up to use i2c mode in the first place, and is using uart mode instead.
}

void ec_setup_sensor() {
  ecChip = new Ezo_board(EC_I2C_ADDR);
  delay(1000);
}

bool sentStatusCheckRequest = false;
void ec_i2c_log_if_error() {
  // switch (ecLastError)
  // {                                // switch case based on what the last response code was.
  // case Ezo_board::errors::SUCCESS: // decimal 1 means the command was successful.
  //   break;                         // exits the switch case.

  // case Ezo_board::errors::FAIL: // decimal 2 means the command has failed.
  //   Serial.print("EC_Error_Command_Failed");
  //   break;                           // exits the switch case.
  // case Ezo_board::errors::NOT_READY: // decimal 254 means the command has not yet been finished calculating.
  //   Serial.println("EC_Error_Command_Pending");
  //   break;                                      // exits the switch case.
  // case Ezo_board::errors::NO_DATA:              // decimal 255.
  //   Serial.println("EC_Error_No_Further_Data"); // means there is no further data to send.
  //   break;                                      // exits the switch case.
  // case Ezo_board::errors::NOT_READ_CMD:         //
  //   Serial.println("EC_Error_NOT_READ_CMD");    //
  //   break;
  //   // default:
  //   //   // sentStatusCheckRequest = true;
  //   //   break;
  //   // another error
  // }
}

float ec_i2c_get_measurement() {
  if (sentStatusCheckRequest == true) {
  } else if (ecChip->is_read_poll() == true) {
    ecLastError = ecChip->receive_read_cmd();
    // check if the last command was
    // ec_i2c_log_if_error();
  }
  if (ecLastError != Ezo_board::errors::NOT_READY) {
    ecChip->send_read_cmd();
  }
  return ecChip->get_last_received_reading();
}

void ec_log_value() {
  Serial.print(F("Conductivity:"));
  sd_log_value(datalogFile, ec_i2c_get_measurement());
}

void ec_i2c_command_mode_loop() {
  Serial.println(F("Entering Electrical Conductivity Board Command Mode... Available Commands:\n"
                   "q - quit/exit ec mode.\n"
                   "Other Commands see: https://atlas-scientific.com/files/EC_EZO_Datasheet.pdf"));
  Serial.println("Live Readings");
  latest_full_cmd[0] = '\0';  // clear the latest full command so that it a: won't accidentally trigger a named command below, b: will trigger the cmd_help_mssg to show because the command is unknown.
  while (true) {

    power_ctrl_check_mag_switch();

    // every second print the raw measured value:
    if (millis() % 1000 == 0) {
      Serial.print("Conductivity_µS/cm:");
      Serial.print(ec_i2c_get_measurement());
      Serial.println(",");
    }

    // check for new characters from serial monitor (user's computer):
    read_serial_input_characters();
    if (!cmd_ready) continue;


    // handle the command:
    Serial.print("received cmd: ");
    Serial.println(latest_full_cmd);
    parse_command();

    /// - / -----------------------
    /// CODE BELOW ADAPTED FROM: https://files.atlas-scientific.com/Ardunio-I2C-EC-sample-code.pdf
    /// - / -----------------------
    char ec_data[32];  // we make a 32 byte character array to hold incoming data from the EC circuit.
    byte in_char = 0;  // used as a 1 byte buffer to store inbound bytes from the EC Circuit.
    byte i = 0;        // counter used for ec_data array.
    int time_ = 570;   // used to change the delay needed depending on the command sent to the EZO Class EC Circuit.

    if (strcmp(latest_cmd_str, "q") == 0)
      break;
    else if (latest_full_cmd[0] == 'c' or latest_full_cmd[0] == 'r')
      time_ = 570;  // if a command has been sent to calibrate or take a reading we wait 570ms so that the circuit has time to take the reading.
    else
      time_ = 250;  // if any other command has been sent we wait only 250ms.

    Wire.beginTransmission(EC_I2C_ADDR);  // call the circuit by its ID number.
    Wire.write(latest_full_cmd);          // transmit the command that was sent through the serial port.
    Wire.endTransmission();               // end the I2C data transmission.

    if (strcmp(latest_full_cmd, "sleep") == 0)
      return;  // if it is the sleep command or quit command, we return (to exit the loop) and do nothing. Issuing a sleep command and then requesting data will wake the EC circuit.

    Serial.println("waiting");
    delay(time_);  // wait the correct amount of time for the circuit to complete its instruction.

    Wire.requestFrom(EC_I2C_ADDR, 32, 1);          // call the circuit and request 32 bytes (this could be too small, but it is the max i2c buffer size for an Arduino)
    ecLastError = (Ezo_board::errors)Wire.read();  // the first byte is the response code, we read this separately.

    ec_i2c_log_if_error();  // log any error that might have occured.

    while (Wire.available()) {  // are there bytes to receive.
      in_char = Wire.read();    // receive a byte.
      ec_data[i] = in_char;     // load this byte into our array.
      i += 1;                   // incur the counter for the array element.
      if (in_char == 0) {       // if we see that we have been sent a null command.
        i = 0;                  // reset the counter i to 0.
        break;                  // exit the while loop.
      }
    }

    Serial.println(ec_data);  // print the response data.
    Serial.println();         // this just makes the output easier to read by adding an extra blank line
  }
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
