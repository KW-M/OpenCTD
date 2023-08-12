// Double include guard
#ifndef BATTERY_FUNCTIONS_H
#define BATTERY_FUNCTIONS_H
#include "ssrov_ctd_pinouts_and_constants.hpp"
#if ENABLE_BATTERY_MONITOR
#include "sdcard.hpp"

#if !defined(VBATPIN) or !defined(REFERENCE_V3_PIN) or !defined(REVERENCE_VOLTAGE_STANDARD) or !defined(BATT_VOLTAGE_DIVIDER_MULTIPLIER) or !defined(MIN_BATT_VOLTS_CUTOFF)
#error "Make sure your pinnouts & constants.h is included first and contains the constants listed in the !defined statements above this error in the source code."
#endif

// ----------------------------------------------
// ------- Battery Montitor Functions -----------
// ----------------------------------------------

void battery_setup();
float battery_get_voltage();
void battery_log_value();
bool battery_voltage_too_low();

#endif
#endif
