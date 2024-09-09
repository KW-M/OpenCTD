#ifndef SD_USB_PASSTHROUGH_H
#define SD_USB_PASSTHROUGH_H


void sd_usb_passthrough_pre_setup();
void sd_usb_passthrough_post_setup();
void sd_usb_passthrough_enable();
void sd_usb_passthrough_disable();
bool sd_usb_passthrough_read_flag_is_set();
void sd_usb_passthrough_clear_read_flag();
bool usb_is_connected();

#endif
