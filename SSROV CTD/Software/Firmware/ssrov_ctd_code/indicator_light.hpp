// Double include guard
#ifndef INDICATOR_LIGHT_FUNCTIONS_H
#define INDICATOR_LIGHT_FUNCTIONS_H

#include "ssrov_ctd_pinouts_and_constants.hpp"
#include "utility_functions.hpp"

#if !defined(STAT1) or !defined(STAT2)
#error "Make sure your pinnouts & constants.h is included first and contains the constants listed in the !defined statements above this error in the source code."
#endif

/// -------------------------------------------------------
/// ------- Indicator light 1 functions -------------------
/// -------------------------------------------------------

void indicator_light_1_on();
void indicator_light_1_off();
void flash_indicator_light_1();
void pulse_indicator_light_1();

/// ------------------------------------------------------------
/// ------- Indicator light 2 functions (if avalable) ----------
/// ------------------------------------------------------------

void indicator_light_2_on();
void indicator_light_2_off();
void flash_indicator_light_2();
void pulse_indicator_light_2();

#endif
