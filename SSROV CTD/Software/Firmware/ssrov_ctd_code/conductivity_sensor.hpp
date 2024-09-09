// Double include guard
#ifndef CONDUCTIVTY_FUNCTIONS_H
#define CONDUCTIVTY_FUNCTIONS_H
#include "ssrov_ctd_pinouts_and_constants.hpp"
#if ENABLE_CONDUCTIVITY_SENSOR
#include "command_mode.hpp"


#if not defined(EC_RX_PIN_NUM) or not defined(EC_TX_PIN_NUM) or not defined(EC_I2C_ADDR)
#error Make sure your pinnouts & constants.h is included first and contains the constants listed in the not defined statements above this error in the source code.
#endif

void ec_switch_from_uart_to_i2c_mode();
void ec_sensor_pre_setup();
void ec_setup_sensor();
bool ec_i2c_log_if_error();
float ec_i2c_get_measurement();
float ec_get_value();

void ec_i2c_show_live_data();
bool ec_i2c_user_command_handler(UserCommand,const __FlashStringHelper *);


#endif
#endif
