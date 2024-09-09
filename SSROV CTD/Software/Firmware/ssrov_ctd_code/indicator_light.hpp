// Double include guard
#ifndef INDICATOR_LIGHT_FUNCTIONS_H
#define INDICATOR_LIGHT_FUNCTIONS_H

#include "ssrov_ctd_pinouts_and_constants.hpp"

#if !defined(LED_STAT1) or !defined(LED_STAT2)
#error "Make sure your pinnouts & constants.h is included first and contains the constants listed in the !defined statements above this error in the source code."
#endif

/// --------------------------------------------------------------------------------------
/// ------- Indicator light functions - compatible with IRQ interrupts -------------------
/// --------------------------------------------------------------------------------------

void indicator_light_on(uint);
void indicator_light_off(uint);
void indicator_light_flash(uint);
void indicator_light_pulse(uint);

#endif
