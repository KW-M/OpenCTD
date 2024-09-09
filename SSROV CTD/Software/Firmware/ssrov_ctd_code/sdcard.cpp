#include "sdcard.hpp"
#include "ssrov_ctd_pinouts_and_constants.hpp"
#include "utility_functions.hpp"
#include "power_control.hpp"
#include "indicator_light.hpp"
#include "clock.hpp"

// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

SdFat sd;
File32 datalogFile;
BufferedPrint<File32, 64> bp;
int current_card_chip_select = MAIN_SDCARD_CHIP_SELECT_PIN;

// Error messages stored in flash.
#define error(msg) sd.errorHalt(F(msg))
#define SD_SPI_MHZ SD_SCK_MHZ(12)  // https://github.com/adafruit/ArduinoCore-samd/pull/186

// -------------------------------------
// ------- SD Card Functions -----------
// -------------------------------------

bool sd_begin(int sd_chip_select_pin) {
  if (not sd.begin(sd_chip_select_pin, SD_SPI_MHZ)) {
    if (sd.card()->errorCode()) {
      println(F(
        "SD initialization failed.\n"
        "Do not reformat the card!\n"
        "Is the card correctly inserted?\n"
        "Is chipSelect set to the correct value?\n"
        "Does another SPI device need to be disabled?\n"
        "Is there a wiring/soldering problem?\n"));
      print(F("SD Error Code: "));
      sd_print_error_code(sd.card()->errorCode());
      print(F(" | SD Error Data: "));
      Serial.println(int(sd.card()->errorData()), HEX);
      return false;
    }
    println(F("\nCard successfully initialized.\n"));
    if (sd.vol()->fatType() == 0) {
      println(F("Can't find a valid FAT16/FAT32 partition.\n"));
      return false;
    }
    println(F("Can't determine error type\n"));
    return false;
  }
  return true;
}

// checks to see if both main & backup sd cards can be read
void sd_check_if_all_cards_are_working() {
  if (sd_begin(MAIN_SDCARD_CHIP_SELECT_PIN)) {
    println(F("Main SD Card found!"));
  } else {
    println(F("Main SD Card/Reader not present or failed"));
  }
  sd.end();
#ifdef BACKUP_SDCARD_CHIP_SELECT_PIN
  // Check for sd card in backup sd card reader
  if (sd_begin(BACKUP_SDCARD_CHIP_SELECT_PIN)) {
    println(F("Backup SD Card found!"));
  } else {
    println(F("Backup SD Card/Reader failed or not present"));
  }
  sd.end();
#endif
}

bool sd_setup_sd_card() {
  /*  Setup the first avalable sd card that works.
      returns false if an sdcard couldn't be found or opened properly.
  */

  // Initialize the sd card reader.
  if (sd_begin(current_card_chip_select)) {
    return true;
  } else {
    sd.end();
#ifdef BACKUP_SDCARD_CHIP_SELECT_PIN
    // if the backup sd card pin is defined in the pinouts and constants file try switching to the backup sd card (or back) and then the loop will run again and call sd_begin
    if (current_card_chip_select == MAIN_SDCARD_CHIP_SELECT_PIN) {
      println(F("Switching to other SD card!"));
      current_card_chip_select = BACKUP_SDCARD_CHIP_SELECT_PIN;
    } else {
      current_card_chip_select = MAIN_SDCARD_CHIP_SELECT_PIN;
    }
#endif
    return false;
  }
}

char *sd_get_folder_path_string(char *stringOutputLocation, const char *folder_ending_path) {
  /* Make a directory that is named by the current year, month, and day.
     Data is stored in this directory.char
     returns the stringOutputLocation that was passed in.
  */
  DateTime date = rtc.now();
  sprintf(stringOutputLocation, "%s/%s/%02d/%02d/", DATA_STORAGE_FOLDER_PATH, folder_ending_path, date.year(), date.month());
  return stringOutputLocation;
}

/**
 * @param stringOutputLocation address of c string  to put a timestamped filename with file extension
   in the format: year_month_day|hours_minutes_seconds.fileExtension
   @returns the stringOutputLocation that was passed in.
*/
char *sd_get_timestamp_filename(char *stringOutputLocation, const char *file_extension) {
  DateTime date = rtc.now();
  // NO LONGER TRUE WITH SdFAT: Filenames must not be longer than 12 characters!! 8.3 (8 in filename, 3 in extension)
  sprintf(stringOutputLocation, "%02d-%02d-%02d_%02d.%02d.%02d.%s", date.year(), date.month(), date.day(), date.hour(), date.minute(), date.second(), file_extension);
  return stringOutputLocation;
}

bool sd_create_folder(const char *folder_path) {
  // check if the folder already exists, return true if it does.
  if (sd.exists(folder_path))
    return true;

  // make the folder (and all child folders in the folder_path)
  bool mkDir_Succsessful = sd.mkdir(folder_path, true);
  if (not mkDir_Succsessful) {
    print(F("Failed to create folder: "));
    Serial.println(folder_path);
  }
  return mkDir_Succsessful;
}

bool sd_open_file(File32 &filePtr, const char *file_path, oflag_t file_open_flags) {
  filePtr.open(file_path, file_open_flags);
  if (not filePtr && sd.exists(file_path)) {
    print(F("Cannot Open File: "));
    println(file_path);
    return false;
  } else if (not filePtr) {
    print(F("Cannot Find or Create File: "));
    println(file_path);
    return false;
  }
  if (not filePtr.sync() or filePtr.getWriteError()) {
    print(F("SD Write Error:"));
    sd_print_error_code(filePtr.getWriteError());
    Serial.println();
    filePtr.close();
    return false;
  }
  return true;
}

bool sd_open_buffered_file(File32 &filePtr, const char *file_path, oflag_t file_open_flags) {
  if (sd_open_file(filePtr, file_path, file_open_flags)) {
    // attatch the "buffered printer" from the "...pinouts_and_constants.h" to the file32 instance (supposed to be more efficient or something)
    bp.begin(&filePtr);
    return true;
  } else {
    // if the file couldn't be opened, close the file and return false.
    return false;
  }
}

void sd_log_string(File32 &filePtr, const char *string) {
  Serial.print(string);
  print(F(","));
  if (not filePtr)
    return;
  bp.print(string);
  bp.print(',');
}

void sd_log_string(File32 &filePtr, const __FlashStringHelper *string) {
  Serial.print(string);
  print(F(","));
  if (not filePtr)
    return;
  bp.print(string);
  bp.print(',');
}

void sd_log_value(File32 &filePtr, float value) {  //
  if (not filePtr)
    return;
  bp.printField(value, ',');
}

bool sd_log_newline(File32 &filePtr) {  //
  if (not filePtr)
    return false;
  Serial.println();
  bp.print('\n');
  bp.sync();                                            // Actually write the File data up to this point onto the file32 object.
  if (not filePtr.sync() or filePtr.getWriteError()) {  // Try to write the File data onto the sdcard.
    print(F("SD Write Error: "));
    sd_print_error_code(filePtr.getWriteError());
    Serial.println();
    return false;
  } else
    return true;
}

/* Setup the SD Card Library to Correctly set the File times as shown in File explorer on a computer (date created & date modified) // Call back for file timestamps.  Only called for file create and sync(). */
void SDCardDateTimeCallback(uint16_t *date, uint16_t *time) {
  DateTime now = rtc.now();

  // Return date using FS_DATE macro to format fields.
  *date = FS_DATE(now.year(), now.month(), now.day());

  // Return time using FS_TIME macro to format fields.
  *time = FS_TIME(now.hour(), now.minute(), now.second());

  // // Return low time bits in units of 10 ms.
  // *ms10 = now.second() & 1 ? 100 : 0;
}

void sd_enable_file_timestamps() {
  // SDCardDateTimeCallback function assumes a gloabl rtcLib instance variable exists called rtc
  FsDateTime::setCallback(SDCardDateTimeCallback);
}

void sd_print_error_code(char errCode) {
  Serial.print(errCode < 16 ? "0X0" : "0X");
  Serial.print(errCode, HEX);
  print(" - ");
  printSdErrorSymbol(&Serial, errCode);
  print(" - ");
  printSdErrorText(&Serial, errCode);
}

// from: https://stackoverflow.com/questions/15763259/unix-timestamp-to-fat-timestamp
DateTime *getDateTimeFromFatTimestamp(uint16_t fatDate, uint16_t fatTime) {
  return new DateTime(FS_YEAR(fatDate), FS_MONTH(fatDate), FS_DAY(fatDate), FS_HOUR(fatTime), FS_MINUTE(fatTime), FS_SECOND(fatTime));
}

DateTime sd_get_file_mod_date(File32 &filePtr) {
  uint16_t fileModificationDate;
  uint16_t fileModificationTime;
  filePtr.getModifyDateTime(&fileModificationDate, &fileModificationTime);
  DateTime fileModDateTime = *getDateTimeFromFatTimestamp(fileModificationDate, fileModificationTime);
  print("File Modification Date: ");
  Serial.print(fileModificationDate);
  print(" ");
  Serial.print(fileModificationTime);
  print(" ");
  Serial.println(fileModDateTime.unixtime());
  return fileModDateTime;
}

void setup_datalog_file(const char *csv_header) {
  // check if the last last_datalog_file_path is a real file
  if (sd.exists(onboard_config.last_datalog_file_path)) {

    sd_open_file(datalogFile, onboard_config.last_datalog_file_path, O_WRONLY | O_APPEND);
    ulong fileAge = (rtc.now() - sd_get_file_mod_date(datalogFile)).totalseconds();
    print("Last datalog modification date:");
    Serial.println(fileAge);
    if (fileAge < ulong(onboard_config.max_seconds_to_resume_writing_to_previous_logfile_after_reboot)) {
      return;  // if the datalog was written to relatively recently (per the config), return. we can just use the datalogfile, which was opened a few lines above.
    }
  }

  // If we are here, either the last_datalog_file_path isn't a real file (such as on first run or sd card wipe), or the last time that file
  // was written to was a long time ago (specified per the config).
  // So make a new log file:
  char filename_str[30];
  char folder_path_str[30 + sizeof DATA_STORAGE_FOLDER_PATH];
  sd_get_timestamp_filename(filename_str, "csv");
  sd_get_folder_path_string(folder_path_str, "datalogs");

  // save the new datalog file path to the config.
  onboard_config.last_datalog_file_path[0] = '\0';                 // reset the last_datalog_file_path to a blank string by setting the first char to the line terminator char
  strcat(onboard_config.last_datalog_file_path, folder_path_str);  // add the folder path to the datalog
  strcat(onboard_config.last_datalog_file_path, filename_str);

  // make the folders that the datalog will go in:
  sd_create_folder(folder_path_str);

  // create the new datalog
  if (datalogFile)
    datalogFile.close();
  sd_open_buffered_file(datalogFile, onboard_config.last_datalog_file_path, O_WRONLY | O_CREAT | O_EXCL);
  sd_log_string(datalogFile, csv_header);
  sd_log_newline(datalogFile);
}

void sd_close() {
  if (datalogFile.isOpen()) datalogFile.close();
  sd.end();
}
