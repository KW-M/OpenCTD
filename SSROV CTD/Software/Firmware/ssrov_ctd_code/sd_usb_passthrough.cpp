#include "delay.h"
#include "sd_usb_passthrough.hpp"
#include <Adafruit_TinyUSB.h>
#include "utility_functions.hpp"

/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!
 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* This example expose SD card as mass storage using
 * SdFat Library
 */

#include "ssrov_ctd_pinouts_and_constants.hpp"
#include "indicator_light.hpp"
#include "sdcard.hpp"

// USB Mass Storage object
Adafruit_USBD_MSC usb_msc;
bool disable_sd_usb_passthrough = false;
bool sd_usb_passthrough_read_flag = false;

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize)
{
    sd_usb_passthrough_read_flag = true;
    if (disable_sd_usb_passthrough)
        return -1;
    uint sectorCount = bufsize / 512;
    bool rc = sd.card()->readSectors(lba, (uint8_t *)buffer, sectorCount);
    return rc ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize)
{

    sd_usb_passthrough_read_flag = true;
    // return -1; // don't actually write; alternatively     // return bufsize;
    if (disable_sd_usb_passthrough)
        return -1;
    uint sectorCount = bufsize / 512;
    bool rc = sd.card()->writeSectors(lba, buffer, sectorCount);
    return rc ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb(void)
{
    sd_usb_passthrough_read_flag = true;
    if (disable_sd_usb_passthrough)
        return;
    sd.card()->syncDevice();

    // clear file system's cache to force refresh
    // sd.vol()->cacheClear();
}


bool msc_start_stop_callback(uint8_t power_condition, bool start, bool load_eject) {
  // Serial.printf("Start/Stop callback: power condition %u, start %u, load_eject %u\n", power_condition, start, load_eject);
  // while (true) {
  //   //  if(start) indicator_light_pulse(LED_STAT2);
  //   //  if(load_eject) indicator_light_flash(LED_STAT2);
  //   //  if(power_condition == 0) indicator_light_flash(LED_STAT3);
  //   //  if(power_condition == 1) indicator_light_flash(LED_STAT2);
  //   //  if(power_condition == 3) indicator_light_flash(LED_STAT1);
  //   //  if(power_condition == 4) indicator_light_pulse(LED_STAT1);
  //    delay(100);
  // }
  return true;
}

// Callback Invoked when received Test Unit Ready command from host.
// return true when SD card isn't busy and has no errors allowing host to read/write
bool msc_ready_callback(void) {
  if(disable_sd_usb_passthrough or sd.card()->isBusy()) return false;
  if (sd.card()->sectorCount() == 0 or sd.card()->errorCode()) {
      print(F("USB Passthrough SD Card error: "));
      sd_print_error_code(sd.card()->errorCode());
      indicator_light_pulse(LED_STAT1);
      return false;
  }
  return true;
}

// Pre setup must be called before Serial.begin() or other usb functions called so the usb controller will know the board will be a storage device.
void sd_usb_passthrough_pre_setup()
{
    // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
    usb_msc.setID("CTD", "SD Card", "1.0");

    // Set read write callback
    usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
    // usb_msc.setStartStopCallback(msc_start_stop_callback);
    // usb_msc.setReadyCallback(msc_ready_callback);

    // Still initialize MSC but tell usb stack that MSC is not ready to read/write
    // If we don't initialize, board will be enumerated as CDC only
    usb_msc.setUnitReady(false);
    usb_msc.begin();
}

// Setup the rest of the sd card passthrough (must be called after real sd card is initilized by sdfat)
void sd_usb_passthrough_post_setup()
{
    // Size in blocks (512 bytes)
    uint32_t sector_count = sd.vol()->sectorsPerCluster() * sd.vol()->clusterCount();

    // Set disk size, SD block size is always 512
    usb_msc.setCapacity(sector_count, 512);

    // MSC is ready for read/write
    sd_usb_passthrough_enable();

    // Show that we should be ready in the serial output
    print("SD Passthrough Ready. Volume size (MB):  ");
    Serial.println((sector_count / 2) / 1024);
}

void sd_usb_passthrough_enable()
{
    msc_flush_cb();
    disable_sd_usb_passthrough = false;
    usb_msc.setUnitReady(true);
}
void sd_usb_passthrough_disable()
{
    msc_flush_cb();
    disable_sd_usb_passthrough = true;
    usb_msc.setUnitReady(false);
}

bool sd_usb_passthrough_read_flag_is_set()
{
    return sd_usb_passthrough_read_flag;
}

void sd_usb_passthrough_clear_read_flag()
{
    sd_usb_passthrough_read_flag = false;
}

bool was_mounted = false;
void tud_umount_cb(void) {
    was_mounted  = true;
}

bool usb_is_connected() {
  return tud_ready();
    if (was_mounted && tud_ready()) { 
        /* PC is connected*/
        return true;
    } else {
       /* PC is disconnected */
        was_mounted = false;
        return false;
    }
}
