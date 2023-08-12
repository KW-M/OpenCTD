/**************************************************************/
/** This file contains Board & Circut Specific Constants & ***/
/** Pinouts (edit if needed) ********************************/
/***********************************************************/

#ifndef SSROV_CTD_PINOUTS_AND_CONSTANTS_H_
#define SSROV_CTD_PINOUTS_AND_CONSTANTS_H_

#define TRUE 1
#define FALSE 0

/* ------------------------------------*/
/* ------ Indicator Lights -----------*/
/* ----------------------------------*/

#define STAT1 LED_BUILTIN // Indicator light 1 (Soldered onto feather board).
#define STAT2 LED_BUILTIN // 12 Indicator light 2 big/normal LED on PCB.

/* -----------------------------------*/
/* ------ Real Time Clock -----------*/
/* ---------------------------------*/

#include "RTClib.h"  // does fit in feather m0 memory
#define RTC_TYPE 'D' //'P' or 'D'
#define RTC_CSV_HEADER "Date_Time,"

/* --------------------------------------*/
/* ------ Battery Monitoring -----------*/
/* ------------------------------------*/

#define ENABLE_BATTERY_MONITOR TRUE
#if ENABLE_BATTERY_MONITOR

#define VBATPIN A7                           // A2 on Arduino Weather Shield, A7 on Adafruit Feather Addalogger
#define REFERENCE_V3_PIN NULL                // A3 on Arduino Weather Shield, A(Doesn't exist?) on Adafruit Feather Addalogger
#define REVERENCE_VOLTAGE_STANDARD 3.3       // The reference voltage is 3.3V on Feathers and the weather shield on UNOs
#define BATT_VOLTAGE_DIVIDER_MULTIPLIER 4.90 // 2 on the Feathers (I think) & 4.90 built into the Weather shield calculated from: (3.9k+1k)/1k
#define MIN_BATT_VOLTS_CUTOFF 5              // voltage at which the batt_voltage_too_low() will return true (and you can make sure to save and shutdown or something...)
#define BATTERY_MONITOR_CSV_HEADER "Battery_Volts,"

#endif

/* -----------------------------------*/
/* ------ SD Card Readers -----------*/
/* ---------------------------------*/

#define DATA_STORAGE_FOLDER_PATH "CTD"   // the folder in the root directory of the sd card to put the files we create (don't include trailing slash)
#define MAX_FILE_SIZE 3900000000         // (in bytes) Technically fat32 supports 4gb files, but I'm putting the cutoff at 3.9 gb.
#define MAIN_SDCARD_CHIP_SELECT_PIN 4    // Addaloger SD Card reader chip select pin is 4, on arduino uno logger shield it's 10.
#define BACKUP_SDCARD_CHIP_SELECT_PIN 10 // no backup card reader ...yet

/* --------------------------------------------------------*/
/* ------ Electrical Conductivity Sensor Board -----------*/
/* ------------------------------------------------------*/

#define ENABLE_CONDUCTIVITY_SENSOR TRUE
#if ENABLE_CONDUCTIVITY_SENSOR

#include <Ezo_i2c.h>
#include <SoftwareSerial.h>
#define EC_RX_PIN_NUM 20 //  scl pin on feather m0
#define EC_TX_PIN_NUM 21 //   sda pin on feather m0
#define EC_I2C_ADDR 100  //  (Default: 100) which i2c address the ec board will use when converted to use i2c.
#define CONDUCTIVITY_SENSOR_CSV_HEADER "Conductivity_Microsiemens/cm,"

#endif

/* --------------------------------------*/
/* ------ Temperature Probes -----------*/
/* ------------------------------------*/

#define ENABLE_TEMP_PROBE TRUE
#if ENABLE_TEMP_PROBE

/* This integer specifies how high resolution you want your temperature sensors to be.
   Ok values: 9,10,11,12 (Higher = more accuracy but slower sampling frequency**)
   * ** There is no reason not to use the highest accuracy. This is because the the frequency of the
   conductivity sensor is the limiting factor [min = 1 second] */
#define TEMP_SENSOR_RESOLUTION 12
#define TEMP_SENSOR_DATA_PIN 6
#define NUM_TEMP_PROBES 3
#define TEMP_PROBE_CSV_HEADER "Temp1_C,Temp2_C,Temp3_C,"

#endif
/* -----------------------------------*/
/* ------ Pressure Sensor -----------*/
/* ---------------------------------*/

#define ENABLE_PRESSURE_SENSOR TRUE
#if ENABLE_PRESSURE_SENSOR

/* This integer specifies how high accuracy you want your pressure sensor to be (oversampling resolution).
   Ok values: 256, 512, 1024, 2048, or 4096 (Higher = more accuracy but slower sampling frequency**)
   * ** There is no reason not to use the highest accuracy. This is because the the frequency of the
   conductivity sensor is the limiting factor [min = 1 second] */
#define PRESSURE_SENSOR_RESOLUTION 4096
#define PRESSURE_SENSOR_CSV_HEADER "Pressure_mBar,"

#endif

/* --------------------------------------*/
/* ------ Orientation IMU Sensor -------*/
/* ------------------------------------*/

#define ENABLE_BNO055_ORIENTATION_SENSOR FALSE
#if ENABLE_BNO055_ORIENTATION_SENSOR

#define BNO055_I2C_ADDRESS 0x28 // Check I2C orientation sensor address and correct line (by default address is 0x29 or 0x28)
#define BNO055_CHIP_ID 55
#define BNO055_SAMPLERATE_DELAY_MS (100) // Set the delay between fresh samples of the orientation sensor (may or may not do anything)
#define DEGREE_TO_RADIAN ((2 * PI) / 360)
#define INCLUDE_CARDINAL_REFERENCE_ORENTATION_FUNCS true
#define BNO055_ORIENTATION_SENSOR_CSV_HEADER "Total_Yaw_Angle_Deg,NS_Lean_Angle_Deg,EW_Lean_Angle_Deg,NS_Acceleration_m/s^2,EW_Acceleration_m/s^2,"

#endif
/* ----------------------------------------------------*/
/* ------ Photoresistor (Analog Light Sensor) --------*/
/* --------------------------------------------------*/

#define ENABLE_PHOTORESISTOR FALSE
#if ENABLE_PHOTORESISTOR

#define PHOTORESISTOR_ENABLE_PIN 12
#define PHOTORESISTOR_SENSE_PIN A4
#define PHOTORESISTOR_SAMPLE_AVERAGING_COUNT 100
#define PHOTORESISTOR_CSV_HEADER "Light_Level_Lux,"

#endif

/* ---------------------------------------------*/
/* ---- Calibration / Saved info Config: ------*/
/* -------------------------------------------*/

#define CONFIG_STORAGE_FOLDER_PATH "/ctd/saved_configurations/"

/*
Each of the below structs groups the configuration and/or calibration values for either the components physically attached to the feather board,
like the photoresistor, or components embeded in the ctd tube, like the temperature probes. They are seperate to allow saved calibration of componenents
to be partially restored, even if the featherwing is plugged into a different tube than it was calibrated with.
   When the feather board boots up, it looks for two files, one for each of the below configs.
   The onboard_config is saved to a file named with the feather's serial number to make sure it is only applied to the same feather board if the sd card is wrongly swapped.
   The tube_components_config is saved to a file named with the one of the temperature probe's serial numbers to make sure it is only applied to the same tube that the calibrations were saved for .

   IF YOU NEED TO EDIT THESE STRUCTS!!
   -  If you can, add the new value to the END of each struct, that will avoid mismatches with config files saved earlier.
   -  If not, make the changes, and then update the CONFIG_VERSION sure to DELETE any existing configs on all SD cards used in the CTDs.
       Then re-upload this code WITHOUT the sd card in the ctd to clear any configs saved in-memory. Finally, re-calibrate the CTDs.
*/

typedef struct
{
   bool log_raw_values = false;                    // default value is false - if true it will only log uncalibrated raw values, bypassing calibration
   unsigned long seconds_between_log_events = 1.0; // default value is 1 log row per second (while you can go lower like 0.25, the e conductivity and temp sensors will still only update at their maximum frequecy of ~1 second).
   // If the feather is rebooted it will try to keep writing to the last open log file unless that logfile was last modified more than the following number of seconds ago:
   int max_seconds_to_resume_writing_to_previous_logfile_after_reboot = 60; // default is 1 minute (60sec)
   char last_datalog_file_path[50] = "";                                    // set to the file path of the datalog file that is currently open, or was open on last boot up. Allows the above mentioned "continue writing" feature.
} onboard_config_type;

// struct tube_components_config_type
// {
//    // Pressure Calibration Values:
//    float pressure_calib_low_measured_value = NAN;
//    float pressure_calib_low_true_value_millipascals = NAN;
//    float pressure_calib_high_measured_value = NAN;
//    float pressure_calib_high_true_value_millipascals = NAN;

//    // Temp Probe Calibration Values:
//    float temp_probe_calib_high_measured_values[NUM_TEMP_PROBES]; // when calibration is run in the (high temp) water bath, the values each temp sensor reports are stored here.
//    float temp_probe_calib_low_measured_values[NUM_TEMP_PROBES];  // when calibration is run in the (low temp) water bath, the values each temp sensor reports are stored here.
//    float temp_probe_calib_high_true_value_c = NAN;                     // when calibration is run in the (high temp) water bath, the true temperature of the bath is stored here in C (we only need one value because we assume the probes all got calibrated in the same bath).
//    float temp_probe_calib_low_true_value_c = NAN;                      // when calibration is run in the (low temp) water bath, the true temperature of the bath is stored here in C (we only ...).
// } tube_components_config;

// Sanity checks to make sure this microcontroller is plugged into the same sensors it has calibrations for, and that the calibration values are correctly formatted.
//  uint8_t saved_config_version_number;    // This must be the first item in the struct and it must be of uint8_t type to work!
//  DeviceAddress known_temp_probe_address; // This stores one of the temp probe's Unique Identifiers, which is used to check that we are plugged into the same ctd tube that the calibrations were performed for!

#endif
