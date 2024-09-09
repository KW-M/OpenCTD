// Double include guard
#ifndef CLOCK_FUNCTIONS_H
#define CLOCK_FUNCTIONS_H

#include "ssrov_ctd_pinouts_and_constants.hpp"

// --------------------------------------------

#if !defined(RTC_TYPE)
#error "Make sure your pinnouts & constants.h is included first and contains the constants listed in the !defined statements above this error in the source code. see weather shield code for an example"
#endif

#if RTC_TYPE == 'D'
extern RTC_DS3231 rtc; // Make an instance variable of the class to talk to the real time clock we are working with (In this case the one on the precision featherwing rtc).
#elif RTC_TYPE == 'P'
extern RTC_PCF8523 rtc; // Make an instance variable of the class to talk to the real time clock we are working with (In this case the one on the adalogger featherwing or arduino uno datalogger shield rtc).
#endif

// -----------------------------------------
// ----- Real Time Clock Functions ---------
// -----------------------------------------

bool clock_setup_rtc();
String clock_get_datetime_string();
void clock_print_time();

#endif
