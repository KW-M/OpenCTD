#include "clock.hpp"

// -----------------------------------
// ------- Global Variables ----------
// -----------------------------------

#if RTC_TYPE == 'D'
RTC_DS3231 rtc; // Make an instance variable of the class to talk to the real time clock we are working with (In this case the one on the precision featherwing rtc).
#elif RTC_TYPE == 'P'
RTC_PCF8523 rtc; // Make an instance variable of the class to talk to the real time clock we are working with (In this case the one on the adalogger featherwing or arduino uno datalogger shield rtc).
#endif

// -----------------------------------------
// ----- Real Time Clock Functions ---------
// -----------------------------------------

bool clock_setup_rtc()
{
  while (not rtc.begin())
  {
    Serial.println(F("Couldn't find Real Time Clock featherwing."));
    pulse_indicator_light_1();
    flash_indicator_light_1();

    power_ctrl_check_mag_switch();
  }

  bool clock_initialized = true;
#if RTC_TYPE == 'P'
  clock_initialized = rtc.initialized();
#endif

  DateTime date_code_was_compiled = DateTime(F(__DATE__), F(__TIME__));

  // Initialize real-time clock
  if (not clock_initialized or rtc.lostPower() or date_code_was_compiled.unixtime() > rtc.now().unixtime())
  {
    Serial.println(F("RTC is NOT initialized, let's set the time!"));
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(date_code_was_compiled);
    // Note: allow 2 seconds after inserting coin battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, rtc.lostPower() may still return true.
  }

#if RTC_TYPE == 'P'
  rtc.start();
#endif

  return true;
}

char *clock_get_datetime_string()
{
  char *outStr = new char[30];
  DateTime date = rtc.now();
  sprintf(outStr, "%02d/%02d/%02d %02d:%02d:%02d", date.month(), date.day(), date.year(), date.hour(), date.minute(), date.second());
  return outStr;
}

void clock_print_time()
{
  char *timeStr = clock_get_datetime_string();
  Serial.print("Current Clock Time: ");
  Serial.println(timeStr);
}
