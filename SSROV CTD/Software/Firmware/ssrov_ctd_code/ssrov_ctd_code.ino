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
#include "photoresistor.hpp"
#include "pressure_sensor.hpp"
#include "orientation_sensor.hpp"


/*  Each tab at the top, and the include statments abvoe corresponds to a file in this folder and those files help organize the different functions.
    We're using .hpp files instead of .ino (though they both just contain code) beacause we can include them in the order we want and they'll all get compiled
    as if all the code in each .hpp file replaced the corresponding #include line. If we use multiple .INO files, the arduino IDE tries to be helpful and compliles them
    in alphabetical order, but sometimes messes up and then the code breaks. If you want to organize the code in a new file, click the dropdown and select new tab,
    but MAKE SURE to ADD ".hpp" at the end and then ADD ANOTHER #include LINE like the ones above for the new tab.
*/

/* You'll need these libraries:
   click the links in the arduino ide to open the library manager (or search for them yourself):
  http://librarymanager/All#SdFat < Use the one by Bill Greiman (NOT the adafruit port (was not updated at time of writing))
  http://librarymanager/All#RTClib < use the one by Adafruit
  http://librarymanager/All#Adafruit_Unified_Sensor
  http://librarymanager/All#Adafruit_BNO055
  You'll also need to download these libraries and install them manually:
  (See this tutorial for how to manually install libraries): https://www.baldengineer.com/installing-arduino-library-from-github.html
  https://github.com/millerlp/MS5803_14
  https://github.com/milesburton/Arduino-Temperature-Control-Library
  https://github.com/OceanographyforEveryone/OpenCTD/tree/master/OpenCTD_Feather_Adalogger/Support_Code/SoftwareSerial
*/
bool isValid = true;
String csv_header = "";
void setup() {

  // Turn on LED to show aliveness
  indicator_light_1_on();

  // power ctrl setup should go as early as possible
  power_ctrl_setup();

  // sd passthrough pre setup MUST go before Serial or SD card setup/initilization.
  sd_usb_passthrough_pre_setup();

// electrical conductivity pre setup MUST GO BEFORE ANY I2C SETUP - Before clock or other sensor setup (because it temporarlly uses the I2C pins as UART pins)
#if ENABLE_CONDUCTIVITY_SENSOR
  ec_sensor_pre_setup();
#endif

  // SETUP SD-CARD -------------------------
  Serial.println(F("Setting up SD Card... "));
  sd_setup_sd_card();               // NOTE: If the sd card is not inserted or failing this function has a loop that will keep trying again until it finds a working sd card.
  sd_usb_passthrough_post_setup();  // continue setting up sd card usb passthrough once sd card is setup.

  // SETUP SERIAL CONNECTION ----- (To send text to / recive text from the serial monitor on a computer) -----
  delay(2000);
  Serial.begin(115200);                                                        // now switch to the faster serial baud rate of 115200;
  while ((not Serial or Serial.availableForWrite() == 0) and millis() < 5000)  // Wait for computer to connect or a 5 second timeout to elapse before continuing.
  {
    delay(10);
    flash_indicator_light_1();
    power_ctrl_check_mag_switch();
  }

  // SETUP CLOCK --------------------------
  Serial.print("Setting up real time clock... ");
  while (not clock_setup_rtc()) {  // if clock setup fails, the clock_setup_rtc() function will return false and this loop will run, pulsing the indicator light & trying again.
    pulse_indicator_light_1();
    power_ctrl_check_mag_switch();
  }
  clock_print_time();
  csv_header += RTC_CSV_HEADER;
  sd_enable_file_timestamps();


  setup_config_storage();
  write_onboard_config();

#if ENABLE_PRESSURE_SENSOR
  Serial.println(F("Setting up pressure sensor..."));
  pressure_setup_sensor();
  csv_header += PRESSURE_SENSOR_CSV_HEADER;
#endif

#if ENABLE_TEMP_PROBE
  Serial.println(F("Setting up temperature probes..."));
  temp_setup_sensors();  // setup all 3 temperature probes
  csv_header += TEMP_PROBE_CSV_HEADER;
#endif

#if ENABLE_CONDUCTIVITY_SENSOR
  Serial.println(F("Setting up electrical conductivity sensor board..."));
  ec_setup_sensor();
  csv_header += CONDUCTIVITY_SENSOR_CSV_HEADER;
#endif

#if ENABLE_PHOTORESISTOR
  Serial.println(F("Setting up light sensor..."));
  photoresistor_setup_sensor();
    csv_header += PHOTORESISTOR_CSV_HEADER;
#endif

#if ENABLE_BNO055_ORIENTATION_SENSOR
  Serial.println(F("Setting up orientation sensor... "));
  orient_setup_sensor();
  csv_header += BNO055_ORIENTATION_SENSOR_CSV_HEADER;
#endif

#if ENABLE_BATTERY_MONITOR
  csv_header += BATTERY_MONITOR_CSV_HEADER;
#endif

  Serial.println(F("==============================================================================="));
  Serial.println(F("To see available commands, send any character from the Arduino Serial Monitor."));
  Serial.println(F("==============================================================================="));

  if (sd_usb_passthrough_read_flag_is_set()) {
    Serial.println(F("SD Card Passthrough Active, Send 'd' to disable and start logging to the sd card."));
    Serial.println();
  }
  Serial.flush();
}

unsigned long lastDatalogTime = 0;

void loop() {

  // check for power off switch hold down.
  power_ctrl_check_mag_switch();

  // Respond to serial commands from the computer...
  read_serial_input_characters();

  // if arduino serial monitor on a connected computer sends aything, run the command mode loop:
  if (cmd_ready) {
    command_mode_loop();
    if (sd_usb_passthrough_read_flag_is_set()) {
      Serial.println(F("SD Card Passthrough Active, Send 'd' to disable and start logging to the sd card."));
      Serial.println();
    }
  }

  if (sd_usb_passthrough_read_flag_is_set()) {
    return;  // do not continue logging while passthrough is active
  }


#if ENABLE_BNO055_ORIENTATION_SENSOR
  // check if the orentation sensor hasn't locked on
  if (not orient_sensor_locked_on()) {
    Serial.print("\n");
    Serial.print(F("Compass not locked on. Rotate the board until light stops flashing. "));
    orient_displayCalibrationStatus();
    flash_indicator_light_1();
  }
#endif

  // don't continue if not enough time has elapsed between the last log event.
  if (millis() < lastDatalogTime + (onboard_config.seconds_between_log_events * 1000))
    return;
  lastDatalogTime = millis();

  // Check if the data log file is working by trying to write the new line character to the datalog:
  // This will be true (ie: not working) on the first loop because the datalogFile isn't open yet, so we open & setup the log file inside the if statement below.

  while (!datalogFile or !sd_log_newline(datalogFile)) {
    Serial.println(F("\nSetting Up Datalog File..."));
    setup_datalog_file(csv_header.c_str());
    pulse_indicator_light_1();
  }
 
  // show a warning if we are set to log RAW / uncalibrated values:
  if (onboard_config.log_raw_values) Serial.print(F("[SHOWING_UNCALIBRATED_VALUES!]"));

  Serial.print(F("Date_Time:"));
  char *time = clock_get_datetime_string();
  sd_log_string(datalogFile, time);  // log the date & time from the real time clock and print it to the console
  delete time;

#if ENABLE_PRESSURE_SENSOR
  pressure_log_value();  // log the pressure sensor value
#endif

#if ENABLE_TEMP_PROBE
  temp_probes_refresh_values();  // ask all the temperature probes to take a new measurement (if they are ready).
  temp_log_value(0);             // write the last measured value of the 1st temp probe to the console
  temp_log_value(1);             // write the last measured value of the 2nd temp probe to the console
  temp_log_value(2);             // write the last measured value of the 3rd temp probe to the console
#endif

#if ENABLE_CONDUCTIVITY_SENSOR
  ec_log_value();
#endif

#if ENABLE_PHOTORESISTOR
  photoresistor_log_value();
#endif

#if ENABLE_BNO055_ORIENTATION_SENSOR
  orient_update_values();  // Ask the orientation sensor/IMU to measure the acceleration/orientation etc. at this point in time.
  orient_log_all_values();
#endif

#if ENABLE_BATTERY_MONITOR
  battery_log_value();
#endif

  Serial.print("Mem:");
  Serial.print(available_memory());
  Serial.print(",");

  flash_indicator_light_2();
}

// /*********************************************************************
//  Adafruit invests time and resources providing this open source code,
//  please support Adafruit and open-source hardware by purchasing
//  products from Adafruit!

//  MIT license, check LICENSE for more information
//  Copyright (c) 2019 Ha Thach for Adafruit Industries
//  All text above, and the splash screen below must be included in
//  any redistribution
// *********************************************************************/

// /* This example expose SD card as mass storage using
//  * SdFat Library
//  */

// #include "SPI.h"
// #include "SdFat.h"
// #include "ssrov_ctd_pinouts_and_constants.hpp"
// #include "sdcard.hpp"
// #include "Adafruit_TinyUSB.h"

// const int chipSelect = 4;

// // File system on SD Card
// // SdFat sd;

// SdFile root;
// SdFile file;

// // USB Mass Storage object
// Adafruit_USBD_MSC usb_msc;

// // Set to true when PC write to flash
// bool changed;

// // the setup function runs once when you press reset or power the board
// void setup()
// {
//   pinMode(LED_BUILTIN, OUTPUT);

//   // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
//   usb_msc.setID("Adafruit", "SD Card", "1.0");

//   // Set read write callback
//   usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);

//   // Still initialize MSC but tell usb stack that MSC is not ready to read/write
//   // If we don't initialize, board will be enumerated as CDC only
//   usb_msc.setUnitReady(false);
//   usb_msc.begin();
//   /*
//    */

//   Serial.begin(115200);
//   // while (!Serial)
//   //   delay(10); // wait for native usb

//   Serial.println("Adafruit TinyUSB Mass Storage SD Card example");

//   Serial.print("\nInitializing SD card ... ");
//   Serial.print("CS = ");
//   Serial.println(chipSelect);

//   if (!sd.begin(chipSelect, SD_SCK_MHZ(12)))
//   {
//     Serial.println("initialization failed. Things to check:");
//     Serial.println("* is a card inserted?");
//     Serial.println("* is your wiring correct?");
//     Serial.println("* did you change the chipSelect pin to match your shield or module?");
//     while (1)
//       delay(1);
//   }

//   // Size in blocks (512 bytes)
//   uint32_t block_count = sd.vol()->sectorsPerCluster() * sd.vol()->clusterCount();

//   Serial.print("Volume size (MB):  ");
//   Serial.println((block_count / 2) / 1024);

//   // Set disk size, SD block size is always 512
//   usb_msc.setCapacity(block_count, 512);

//   // MSC is ready for read/write
//   usb_msc.setUnitReady(true);

//   changed = true; // to print contents initially
// }

// void loop()
// {
//   if (changed)
//   {
//     root.open("/");
//     Serial.println("SD contents:");

//     // Open next file in root.
//     // Warning, openNext starts at the current directory position
//     // so a rewind of the directory may be required.
//     while (file.openNext(&root, O_RDONLY))
//     {
//       file.printFileSize(&Serial);
//       Serial.write(' ');
//       file.printName(&Serial);
//       if (file.isDir())
//       {
//         // Indicate a directory.
//         Serial.write('/');
//       }
//       Serial.println();
//       file.close();
//     }

//     root.close();

//     Serial.println();

//     changed = false;
//     delay(1000); // refresh every 0.5 second
//   }
// }

// // Callback invoked when received READ10 command.
// // Copy disk's data to buffer (up to bufsize) and
// // return number of copied bytes (must be multiple of block size)
// int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize)
// {
//   return sd.card()->readSectors(lba, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
// }

// // Callback invoked when received WRITE10 command.
// // Process data in buffer to disk's storage and
// // return number of written bytes (must be multiple of block size)
// int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize)
// {
//   digitalWrite(LED_BUILTIN, HIGH);

//   return sd.card()->writeSectors(lba, buffer, bufsize / 512) ? bufsize : -1;
// }

// // Callback invoked when WRITE10 command is completed (status received and accepted by host).
// // used to flush any pending cache.
// void msc_flush_cb(void)
// {
//   sd.card()->syncDevice();

//   // clear file system's cache to force refresh
//   // sd.vol()

//   changed = true;

//   digitalWrite(LED_BUILTIN, LOW);
// }
