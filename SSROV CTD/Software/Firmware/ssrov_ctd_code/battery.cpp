#include "battery.hpp"
#if ENABLE_BATTERY_MONITOR

// ----------------------------------------------
// ------- Battery Montitor Functions -----------
// ----------------------------------------------

void battery_setup()
{
#if REFERENCE_V3_PIN != NULL
  pinMode(REFERENCE_V3_PIN, INPUT);
#endif
  pinMode(VBATPIN, INPUT);
}

// Returns the voltage of the raw pin based on the 3.3V rail
// This allows us to ignore what VCC might be (an Arduino plugged into USB has VCC of 4.5 to 5.2V)
// Battery level is connected to the RAW (VIN) pin on Arduino and (If the weather shield is plugged in) is fed through two 5% resistors:
// 3.9K on the high side (R1), and 1K on the low side (R2)
float battery_get_voltage()
{
  float operatingVoltage = 1023; //
#if REFERENCE_V3_PIN != NULL
  operatingVoltage = analogRead(REFERENCE_V3_PIN);
#endif
  float rawVoltage = analogRead(VBATPIN);
  //  Serial.print(rawVoltage); Serial.print("*"); Serial.print(operatingVoltage);  Serial.print("*"); Serial.print(BATT_VOLTAGE_DIVIDER_MULTIPLIER) Serial.print("=");
  operatingVoltage = REVERENCE_VOLTAGE_STANDARD / operatingVoltage; // The reference voltage is 3.3V - divide adjusts for lower operating Voltages
  rawVoltage = operatingVoltage * rawVoltage;                       // Convert the 0 to 1023 int from the analogRead() to actual voltage on BATT pin (this happens because both operatingVoltage & rawVoltage are on the 0 to 1023 scale, so we're taking the ratio)
  rawVoltage *= BATT_VOLTAGE_DIVIDER_MULTIPLIER;                    // multiply BATT voltage by the voltage divider to get actual system voltage
  return rawVoltage;
}

void battery_log_value()
{
  Serial.print(F("BattVolts:"));
  sd_log_value(datalogFile, battery_get_voltage());
}

bool battery_voltage_too_low()
{
  // returns false if the battery voltage is less than the min cutoff, otherwise returns true
  return (battery_get_voltage() < MIN_BATT_VOLTS_CUTOFF);
}

#endif