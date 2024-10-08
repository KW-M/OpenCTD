// Double include guard
#ifndef ORIENTATION_SENSOR_H
#define ORIENTATION_SENSOR_H
#include "ssrov_ctd_pinouts_and_constants.hpp"
#if ENABLE_BNO055_ORIENTATION_SENSOR


#if !defined(VBATPIN) or !defined(REFERENCE_V3_PIN) or !defined(MIN_BATT_VOLTS_CUTOFF)
#error "Make sure your pinnouts & constants.h is included first and contains the constants listed in the !defined statements on the first line of this file."
#endif

extern bool orient_sensor_has_locked_on;

// ----------------------------------------
// --- Orientation Sensor Functions  ------
// ----------------------------------------

#define ORIENT_SENSOR_SETUP_TIMEOUT_COUNT 4
void orient_setup_sensor();
bool orient_sensor_locked_on();
void orient_get_orientation_reading();
void orient_get_accel_reading();
void orient_update_values();
float orient_get_NS_pitch();
float orient_get_EW_pitch();
float orient_get_NS_accel();
float orient_get_EW_accel();
float orient_get_cumulative_yaw_angle();
void orient_displaySensorDetails(void);
void orient_displaySensorStatus(void);
void orient_displayCalibrationStatus(void);

#endif
#endif
