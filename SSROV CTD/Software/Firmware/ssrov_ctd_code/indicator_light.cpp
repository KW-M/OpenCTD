#include "indicator_light.hpp"
#include "utility_functions.hpp"
#include "variant.h"
#include "delay.h"
/// ------------------------------------------------------------
/// ------- Indicator light 1 functions (if avalable) ----------
/// ------------------------------------------------------------

// void disable_lights



void indicator_light_on(uint pin_num)
{
  pinMode(pin_num, OUTPUT); 
  digitalWrite(pin_num, HIGH); // Turn on LED
}

void indicator_light_off(uint pin_num)
{
  digitalWrite(pin_num, LOW); // Turn off LED
}

void indicator_light_flash(uint pin_num)
{
  
  indicator_light_on(pin_num); // Turn on LED
  delayMicroseconds(50000);
  indicator_light_off(pin_num); // Turn off LED
  delayMicroseconds(50);
}

void indicator_light_pulse(uint pin_num)
{
  int maxBrightness = 20;
  // fade in
  for (int i = 3; i <= maxBrightness; i += 3)
  {
    indicator_light_on(pin_num); // Turn on LED
    delayMicroseconds(i*1000);
    indicator_light_off(pin_num); // Turn off LED
    delayMicroseconds((maxBrightness - i)*1000);
  }

  // keep on
  indicator_light_on(pin_num);
  delayMicroseconds(maxBrightness * 1000);

// fade out
  for (int i = maxBrightness; i > 3; i -= 3)
  {
    indicator_light_on(pin_num); // Turn on LED
    delayMicroseconds(i*1000);
    indicator_light_off(pin_num); // Turn off LED
    delayMicroseconds((maxBrightness - i)*1000);
  }

  indicator_light_off(pin_num); // Turn off LED
}

