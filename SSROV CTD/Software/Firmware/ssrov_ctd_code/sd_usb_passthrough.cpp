#include "sd_usb_passthrough.hpp"

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
    return sd.card()->readSectors(lba, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
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
    return sd.card()->writeSectors(lba, buffer, bufsize / 512) ? bufsize : -1;
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

// Pre setup must be called before Serial.begin() or other usb functions called so the usb controller will know the board will be a storage device.
void sd_usb_passthrough_pre_setup()
{
    // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
    usb_msc.setID("CTD", "SD Card", "1.0");

    // Set read write callback
    usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);

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
    // Serial.print("SD Passthrough Ready. Volume size (MB):  ");
    // Serial.println((sector_count / 2) / 1024);
}

void sd_usb_passthrough_enable()
{
    disable_sd_usb_passthrough = false;
    usb_msc.setUnitReady(true);
}
void sd_usb_passthrough_disable()
{
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
