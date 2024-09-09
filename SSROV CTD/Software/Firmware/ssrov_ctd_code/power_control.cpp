#include "delay.h"
#include "Arduino.h"
#include "wiring_digital.h"
#include <wiring_private.h>
#include "interupt_timer.hpp"
#include "SERCOM.h"
// #include <cmath>
// #include "Arduino.h"
// #include "WInterrupts.h"
// #include "delay.h"
// #include "wiring_digital.h"
// #include "Adafruit_USBD_CDC.h"

#include "power_control.hpp"
#include "indicator_light.hpp"

#define POWER_CTRL_SENSE_PIN A5
#define POWER_CTRL_OUTPUT_PIN 11
#define POWER_CTRL_HOLD_ON HIGH
#define POWER_CTRL_HOLD_OFF LOW
#define POWER_CTRL_ONOFF_HOLD_DELAY 1000  //ms
#define POWER_CTRL_ON_THRESHOLD_VOLTAGE 2.0

float power_ctrl_start_voltage = 0;
volatile long unsigned int power_ctrl_hold_start_time = 0;
volatile bool powerup_hold_delay_complete = false;
volatile bool switch_has_been_released_flag = false;
volatile bool isTimerLedOn = false;
volatile bool board_will_power_down = false;
volatile int i2c_idle_counter = 3;

void power_ctrl_setup() {
  indicator_light_on(LED_STAT3);
  pinMode(POWER_CTRL_OUTPUT_PIN, OUTPUT);
  pinMode(POWER_CTRL_SENSE_PIN, INPUT); // changed
  digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_ON);  // KEEP BOARD ON
  power_ctrl_hold_start_time = millis();
  power_ctrl_start_voltage = power_ctrl_get_switch_voltage();
  // startTimer(6, power_ctrl_check_switch); //CAUSES RANDOM HANGS!
}

inline float power_ctrl_get_switch_voltage() {
  analogRead(POWER_CTRL_SENSE_PIN);  // Warm-up read - throw away.
  volatile float raw_value = (float)analogRead(POWER_CTRL_SENSE_PIN);
  return 3.33 * raw_value / 1023.0;
}

void power_ctl_show_mag_state(bool flash) {
  if(PERIPH_WIRE.isBusIdleWIRE()) {
    i2c_idle_counter += 1;
  } else {
    i2c_idle_counter = 0;
  }

  if (!flash) {
    indicator_light_off(LED_STAT1);
    indicator_light_off(LED_STAT2);
    indicator_light_off(LED_STAT3);
    return;
  } else {
    indicator_light_on(LED_STAT3);
  }
  // if(PERIPH_WIRE.isBusBusyWIRE()) {
    // WAS OFF
  //   indicator_light_on(LED_STAT1);
  // }
  // if(!SERCOM3->I2CM.INTFLAG.bit.MB) {
    // WAS ON
  //   indicator_light_on(LED_STAT2);
  // }  
  // if(!PERIPH_WIRE.availableWIRE()) {
    // Was ON
  //   indicator_light_on(LED_STAT3);
  // }

  // if(PERIPH_WIRE.isStopDetectedWIRE()) {
    // was OFF
  //   //indicator_light_on(LED_STAT2);
  // }  
  // if(PERIPH_WIRE.isBusUnknownWIRE()) {
    // Was OFF
  //   indicator_light_on(LED_STAT3);
  // }



  if(PERIPH_WIRE.isArbLostWIRE()) {
    indicator_light_on(LED_STAT2);
  }  
  // if(PERIPH_WIRE.isMasterReadOperationWIRE()) {
  //   indicator_light_on(LED_STAT3);
  // }


  if (i2c_idle_counter >= 10) {
    indicator_light_on(LED_STAT1);
    Wire.end(); 
    Wire.begin(); // < attempt to restart the I2C system
    i2c_idle_counter = 0;
  } else {
    indicator_light_off(LED_STAT1);
  }

}

unsigned long lastPowerCheckTime = 0;
void power_ctrl_check_switch() {
  if (millis() < lastPowerCheckTime + 200) return;
  lastPowerCheckTime = millis();

  // Read the state of the magnet switch:
  volatile float switch_voltage = power_ctrl_get_switch_voltage();
  if (switch_voltage > POWER_CTRL_ON_THRESHOLD_VOLTAGE) {
    if (millis() > power_ctrl_hold_start_time + POWER_CTRL_ONOFF_HOLD_DELAY) {
      if (switch_has_been_released_flag) {
        digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_OFF);  // TURN OFF BOARD.
        board_will_power_down = true;
        isTimerLedOn = false;
      }      
    } else {
      isTimerLedOn = !isTimerLedOn;
    }
    if (!board_will_power_down) indicator_light_on(LED_STAT2);
  } else {
    indicator_light_off(LED_STAT2);
    power_ctrl_hold_start_time = millis();  // KEEP RESETING THE HOLD START TIME WHILE THE POWER SWITCH IS NOT PRESSED.
    switch_has_been_released_flag = true;
    isTimerLedOn = false;
  }
  //power_ctl_show_mag_state(isTimerLedOn);
    if (isTimerLedOn && !board_will_power_down) indicator_light_on(LED_STAT3); else indicator_light_off(LED_STAT3);
}



// volatile float sw_voltages[4000] = { 0 };

// float power_ctrl_get_switch_voltage() {
//   // pinMode(POWER_CTRL_SENSE_PIN, INPUT_PULLUP);
//   // pinMode(POWER_CTRL_SENSE_PIN, INPUT_PULLUP);
//   analogRead(POWER_CTRL_SENSE_PIN);  // Warm-up read.
//   float raw_value = (float)analogRead(POWER_CTRL_SENSE_PIN);
//   return 3.33 * raw_value / 1023.0;
// }

// void resetSwitchInterrupts();


// #define SWITCH_ON_THRESHOLD 2.0
// void onPowerSwitchPressed() {
//   detachInterrupt(digitalPinToInterrupt(A5));
//   indicator_light_on(LED_STAT3);
//   delayMicroseconds(10000);           // 10 ms
//   pinMode(POWER_CTRL_SENSE_PIN, INPUT); //CHANGE

//   uint i = 0;
//   float voltage;
//   float lastVoltage = infinityf();
//   bool flipflop = false;
//   analogRead(POWER_CTRL_SENSE_PIN);  // Warm-up read.
//   analogRead(POWER_CTRL_SENSE_PIN);  // Warm-up read.

//   while (i < POWER_CTRL_ONOFF_HOLD_DELAY) {
//     // Flash Light:
//     if (i % 80 == 0) flipflop = !flipflop;
//     if (flipflop) indicator_light_on(LED_STAT3);
//     else indicator_light_off(LED_STAT3);

//     // Wait a bit:
//     delayMicroseconds(1000);           // 1 ms

//     // Check the switch state:
//     analogRead(POWER_CTRL_SENSE_PIN);  // Warm-up read.
//     float raw_value = (float)analogRead(POWER_CTRL_SENSE_PIN);
//     voltage = 3.33 * raw_value / 1023.0;

//     // break out of loop if switch turns off in the middle of waiting:
//     //  && lastVoltage < SWITCH_ON_THRESHOLD
//     if (voltage < SWITCH_ON_THRESHOLD) {
//       indicator_light_off(3);
//       switch_has_been_released_flag = true;
//       attachInterrupt(digitalPinToInterrupt(A5), onPowerSwitchPressed, RISING);
//       return; 
//     } else {
//       indicator_light_on(LED_STAT3);
//     }
//     lastVoltage = voltage;
//     // sw_voltages[i] = voltage;
//     // if (voltage < minvoltage) minvoltage = voltage;
//     // if (voltage > maxvoltage) maxvoltage = voltage;
//     i++;
//   }
//   pinMode(POWER_CTRL_SENSE_PIN, INPUT);
//   if (switch_has_been_released_flag) {
//     digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_OFF);
//     // indicator_light_off(LED_STAT3);

//   }
//   // } else {
//      attachInterrupt(digitalPinToInterrupt(A5), onPowerSwitchPressed, CHANGE);
  
//   // print("Power Switch ON=");
//   // print("MS");
//   // Serial.println(millis());
//   // println("minv");
//   // Serial.println(minvoltage);
//   // println("maxv");
//   // Serial.println(maxvoltage);
//   // 
//   // interrupts();
// }


// void resetSwitchInterrupts() {
//   detachInterrupt(digitalPinToInterrupt(A5));
//   attachInterrupt(digitalPinToInterrupt(A5), onPowerSwitchPressed, RISING);
// }

// void power_ctrl_setup() {
//   digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_ON);  // KEEP BOARD ON
//   // pinMode(A4, OUTPUT);
//   pinMode(POWER_CTRL_OUTPUT_PIN, OUTPUT);
//   pinMode(POWER_CTRL_SENSE_PIN, INPUT); // changed
//   power_ctrl_hold_start_time = millis();
//   // digitalWrite(A4, 1);
//   power_ctrl_start_voltage = power_ctrl_get_switch_voltage();


//   // println("Switching ON...");
//   attachInterrupt(digitalPinToInterrupt(A5), onPowerSwitchPressed, CHANGE);
//   //  attachInterrupt(digitalPinToInterrupt(A5), onPowerSwitchReleased, FALLING);
// }

// void // power_ctrl_check_switch(); {
//   return;
//   // // check if the magnet switch has been held down long enough to count this as a true "power on" vs just a blip signal:
//   // if (!powerup_hold_delay_complete && millis() - power_ctrl_hold_start_time > POWER_CTRL_ONOFF_HOLD_DELAY) {

//   //   powerup_hold_delay_complete = true;
//   //   digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_ON);  // KEEP BOARD ON

//   //   // If we're already on, keep checking for the magnet switch to change:
//   // } else if (powerup_hold_delay_complete) {

//   // Read the state of the magnet switch:
//   float switch_voltage = power_ctrl_get_switch_voltage();
//   // print("power_ctrl_start_voltage:");
//   // Serial.print(power_ctrl_start_voltage);
//   // print(" switch_voltage:");
//   // Serial.print(switch_voltage);
//   // print(" diff:");
//   // Serial.println(power_ctrl_start_voltage - switch_voltage);
//   if (switch_voltage > 3.0) {
//     if (millis() > power_ctrl_hold_start_time + POWER_CTRL_ONOFF_HOLD_DELAY) {
//       indicator_light_flash(LED_STAT1);

//       if (switch_has_been_released_flag) {
//         digitalWrite(POWER_CTRL_OUTPUT_PIN, POWER_CTRL_HOLD_OFF);  // TURN OFF BOARD.
//       }
//     } else {
//       indicator_light_on(LED_STAT1);
//     }
//   } else {
//     power_ctrl_hold_start_time = millis();  // KEEP RESETING THE HOLD START TIME WHILE THE POWER SWITCH IS NOT PRESSED.
//     switch_has_been_released_flag = true;
//     indicator_light_off(LED_STAT1);
//   }
//   // } else {
//   //   indicator_light_on(LED_STAT1);
//   // }
// }