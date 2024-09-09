// Include guard
#ifndef SD_FUNCTIONS_H
#define SD_FUNCTIONS_H

#include <Arduino.h>
#include <BufferedPrint.h>
#include <SdFat.h>
#include <SdFatConfig.h>

#include "ssrov_ctd_pinouts_and_constants.hpp"
#include "clock.hpp"

#if !defined(DATA_STORAGE_FOLDER_PATH) or !defined(MAX_FILE_SIZE) or !defined(MAIN_SDCARD_CHIP_SELECT_PIN)
#error "Make sure your pinnouts & constants.h is included first and contains the constants listed in the !defined statements above this error in the source code. see weather shield code for an example"
#endif

// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

extern SdFat sd;
extern File32 datalogFile;
extern BufferedPrint<File32, 64> bp;
extern onboard_config_type default_onboard_config;
extern onboard_config_type onboard_config;

#define error(msg) sd.errorHalt(F(msg)) // Store sd error messages in flash.
#define SD_SPI_MHZ SD_SCK_MHZ(12)       // https://github.com/adafruit/ArduinoCore-samd/pull/186

// -------------------------------------
// ------- SD Card Functions -----------
// -------------------------------------

void sd_print_error_code(char errCode);
DateTime *getDateTimeFromFatTimestamp(uint16_t fatDate, uint16_t fatTime);
bool sd_begin(int sd_chip_select_pin);
void sd_find_working_sdcard();
bool sd_setup_sd_card();
char *sd_get_folder_path_string(char *stringOutputLocation, const char *folder_ending_path);
char *sd_get_timestamp_filename(char *stringOutputLocation, const char *file_extension);
bool sd_create_folder(const char *folder_path);
bool sd_open_file(File32 &filePtr, const char *file_path, oflag_t file_open_flags);
bool sd_open_buffered_file(File32 &filePtr, const char *file_path, oflag_t file_open_flags);
void sd_log_string(File32 &datalogFile, const char *string);
void sd_log_string(File32 &datalogFile, const __FlashStringHelper *string);
void sd_log_value(File32 &datalogFile, float value);
bool sd_log_newline(File32 &datalogFile);
void SDCardDateTimeCallback(uint16_t *date, uint16_t *time);
void sd_enable_file_timestamps();
void setup_datalog_file(const char *csv_header);
void print_sd_card_error(const char *filePath);
void sd_close();

#endif
