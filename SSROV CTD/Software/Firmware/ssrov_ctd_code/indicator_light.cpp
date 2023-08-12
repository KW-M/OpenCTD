// Double include guard
#include "indicator_light.hpp"

/// ------------------------------------------------------------
/// ------- Indicator light 1 functions (if avalable) ----------
/// ------------------------------------------------------------

void indicator_light_1_on()
{
  pinMode(STAT1, OUTPUT);    // Status LED Blue
  digitalWrite(STAT1, HIGH); // Turn on stat LED
}

void indicator_light_1_off()
{
  digitalWrite(STAT1, LOW); // Turn off stat LED
}

void flash_indicator_light_1()
{
  indicator_light_1_on(); // Turn on stat LED
  delay(50);
  indicator_light_1_off(); // Turn off stat LED
  delay(50);
}

void pulse_indicator_light_1()
{
  int maxBrightness = 20;
  for (int i = 3; i <= maxBrightness; i += 3)
  {
    indicator_light_1_on(); // Turn on stat LED
    delay(i);
    indicator_light_1_off(); // Turn off stat LED
    delay(maxBrightness - i);
  }

  indicator_light_1_on();
  delay(maxBrightness);

  for (int i = maxBrightness; i > 3; i -= 3)
  {
    indicator_light_1_on(); // Turn on stat LED
    delay(i);
    indicator_light_1_off(); // Turn off stat LED
    delay(maxBrightness - i);
  }

  indicator_light_1_off(); // Turn off stat LED
}

/// ------------------------------------------------------------
/// ------- Indicator light 2 functions (if avalable) ----------
/// ------------------------------------------------------------

void indicator_light_2_on()
{
  pinMode(STAT2, OUTPUT);    // Status LED Green
  digitalWrite(STAT2, HIGH); // Turn on stat LED
}

void indicator_light_2_off()
{
  digitalWrite(STAT2, LOW); // Turn off stat LED
}

void flash_indicator_light_2()
{
  indicator_light_2_on(); // Turn on stat LED
  delay(50);
  indicator_light_2_off(); // Turn off stat LED
  delay(50);
}

void pulse_indicator_light_2()
{
  int maxBrightness = 20;
  for (int i = 3; i <= maxBrightness; i += 3)
  {
    indicator_light_2_on(); // Turn on stat LED
    delay(i);
    indicator_light_2_off(); // Turn off stat LED
    delay(maxBrightness - i);
  }

  indicator_light_2_on();
  delay(maxBrightness);

  for (int i = maxBrightness; i > 3; i -= 3)
  {
    indicator_light_2_on(); // Turn on stat LED
    delay(i);
    indicator_light_2_off(); // Turn off stat LED
    delay(maxBrightness - i);
  }

  indicator_light_2_off(); // Turn off stat LED
}
