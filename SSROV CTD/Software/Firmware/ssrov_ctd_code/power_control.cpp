#include "wiring_digital.h"
#include "Adafruit_USBD_CDC.h"

#include "power_control.hpp"
#include "indicator_light.hpp"

#define POWER_CTRL_INPUT_PIN A5
#define POWER_CTRL_OUTPUT_PIN 11
#define POWER_CTRL_ONOFF_HOLD_DELAY 4000  //ms
#define POWER_CTRL_HOLD_ON HIGH
#define POWER_CTRL_HOLD_OFF LOW

long unsigned int power_ctrl_hold_start_time = 0;
float power_ctrl_start_voltage = 0;
bool powerup_hold_delay_complete = false;
bool switch_has_been_released_flag = false;

float get_switch_voltage() {
  pinMode(POWER_CTRL_INPUT_PIN, INPUT_PULLUP);
  pinMode(POWER_CTRL_INPUT_PIN, INPUT_PULLUP);
  analogRead(POWER_CTRL_INPUT_PIN);  // Warm-up read.
  float raw_value = (float)analogRead(POWER_CTRL_INPUT_PIN);
  return 3.3 * raw_value / 1023.0;
}

void power_ctrl_setup() {
  digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_OFF);
  pinMode(A4, OUTPUT);
  pinMode(POWER_CTRL_OUTPUT_PIN, OUTPUT);
  pinMode(POWER_CTRL_INPUT_PIN, INPUT_PULLUP);
  power_ctrl_hold_start_time = millis();
  digitalWrite(A4, 1); 
  power_ctrl_start_voltage = get_switch_voltage();
  digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_ON);  // KEEP BOARD ON
}

void power_ctrl_check_mag_switch() {

  // // check if the magnet switch has been held down long enough to count this as a true "power on" vs just a blip signal:
  // if (!powerup_hold_delay_complete && millis() - power_ctrl_hold_start_time > POWER_CTRL_ONOFF_HOLD_DELAY) {

  //   powerup_hold_delay_complete = true;
  //   digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_ON);  // KEEP BOARD ON

  //   // If we're already on, keep checking for the magnet switch to change:
  // } else if (powerup_hold_delay_complete) {

  // Read the state of the magnet switch:
  float switch_voltage = get_switch_voltage();
  // Serial.print("power_ctrl_start_voltage:");
  // Serial.print(power_ctrl_start_voltage);
  // Serial.print(" switch_voltage:");
  // Serial.print(switch_voltage);
  // Serial.print(" diff:");
  // Serial.println(power_ctrl_start_voltage - switch_voltage);
  if (switch_voltage > 3.0) {
    if (millis() > power_ctrl_hold_start_time + POWER_CTRL_ONOFF_HOLD_DELAY) {
      flash_indicator_light_1();
      if (switch_has_been_released_flag) digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_OFF);  // TURN OFF BOARD.
    } else {
      indicator_light_1_on();
    }
  } else {
    power_ctrl_hold_start_time = millis();  // KEEP RESETING THE HOLD START TIME WHILE THE POWER SWITCH IS NOT PRESSED.
    switch_has_been_released_flag = true;
    indicator_light_1_off();
  }
  // } else {
  //   indicator_light_1_on();
  // }
}