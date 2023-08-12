#if ENABLE_BNO055_ORIENTATION_SENSOR
#include "orientation_sensor.hpp"

// -------------------------------------
// -------- Global Variables -----------
// -------------------------------------

Adafruit_BNO055 bno = Adafruit_BNO055(BNO055_CHIP_ID, BNO055_I2C_ADDRESS);
bool sensor_has_locked_on = false;
float yaw_angle;
float pitch_angle;
float roll_angle;
float accel_x;
float accel_y;
float accel_z;
int cumulative_yaw_angle; // (derived from yaw_angle value):

// ----------------------------------------
// --- Orientation Sensor Functions  ------
// ----------------------------------------

void orient_setup_sensor()
{
  int counter = ORIENT_SENSOR_SETUP_TIMEOUT_COUNT;
  while (not bno.begin() and counter > 0)
  { // initilize the orientation sensor
    Serial.print("No BNO055 orientation sensor detected | Continuing in: ");
    Serial.println(counter--);
    Serial.println(" -- If one is installed, check your wiring & the sensor's I2C address.");
    delay(500); // Delay the program for half a second if the orientation sensor isn't found.
    power_ctrl_check_mag_switch();
  }
  if (counter == 0)
  {
    Serial.println();
    return;
  }

  bno.setExtCrystalUse(true); // tell the orientation sensor to use the external timing crystal

  if (Serial)
  {
    orient_displaySensorDetails(); /* Display some basic information on this sensor */
    orient_displaySensorStatus();  /* Optional: Display current status */
  }

  delay(250); // Wait a quarter second to continue.
}

bool orient_sensor_locked_on()
{
  uint8_t sysLock, gyro, accel, mag;
  sysLock = gyro = accel = mag = 0;
  bno.getCalibration(&sysLock, &gyro, &accel, &mag);
  sensor_has_locked_on = (sysLock != 0);
  return sensor_has_locked_on;
}

void orient_get_orientation_reading()
{
  sensors_event_t orientationData;
  bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
  yaw_angle = orientationData.orientation.x;
  pitch_angle = orientationData.orientation.y;
  roll_angle = orientationData.orientation.z;
}

void orient_get_accel_reading()
{
  sensors_event_t linearAccelData;
  bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
  accel_x = linearAccelData.acceleration.x;
  accel_y = linearAccelData.acceleration.y;
  accel_z = linearAccelData.acceleration.z;
}

void orient_update_values()
{
  if (not sensor_has_locked_on)
    orient_sensor_locked_on();
  orient_get_orientation_reading();
  orient_get_accel_reading();
}

float orient_get_NS_pitch()
{
  if (not sensor_has_locked_on)
    return pitch_angle;
  float yawCos = cos(yaw_angle * DEGREE_TO_RADIAN);
  float yawCos90 = cos((yaw_angle + 90) * DEGREE_TO_RADIAN);

  return -yawCos * pitch_angle - yawCos90 * roll_angle;
}

float orient_get_EW_pitch()
{
  if (not sensor_has_locked_on)
    return roll_angle;
  float yawSin = sin(yaw_angle * DEGREE_TO_RADIAN);
  float yawSin90 = sin((yaw_angle + 90) * DEGREE_TO_RADIAN);

  return -yawSin * pitch_angle - yawSin90 * roll_angle;
}

float orient_get_NS_accel()
{
  if (not sensor_has_locked_on)
    return accel_y;
  float yawCos = cos(yaw_angle * DEGREE_TO_RADIAN);
  float yawCos90 = cos((yaw_angle + 90) * DEGREE_TO_RADIAN);

  return -yawCos * accel_x - yawCos90 * accel_y;
}

float orient_get_EW_accel()
{
  if (not sensor_has_locked_on)
    return accel_x;
  float yawSin = sin(yaw_angle * DEGREE_TO_RADIAN);
  float yawSin90 = sin((yaw_angle + 90) * DEGREE_TO_RADIAN);

  return -yawSin * accel_x - yawSin90 * accel_y;
}

int16_t last_yaw_angle = 0;
float orient_get_cumulative_yaw_angle()
{
  if (not sensor_has_locked_on)
    return NAN; // Return "Not a Number"
  // calculate the change in yaw angle accounting for 360->0 degree wraparound
  int d = abs(int(yaw_angle) - last_yaw_angle) % 360;
  int delta_yaw = d > 180 ? 360 - d : d;
  // calculate sign of yaw delta
  int sign = (yaw_angle - last_yaw_angle >= 0 and yaw_angle - last_yaw_angle <= 180) or (yaw_angle - last_yaw_angle <= -180 and yaw_angle - last_yaw_angle >= -360) ? 1 : -1;
  cumulative_yaw_angle += delta_yaw * sign;
  last_yaw_angle = int(yaw_angle);
  return cumulative_yaw_angle;
}

/****** Below are orientation sensor debug functions made by Adafruit ********/

/**************************************************************************/
/*  Displays some basic information on this sensor (orientation) from the unified
    sensor API sensor_t type (see Adafruit_Sensor for more information) */
/**************************************************************************/
void orient_displaySensorDetails(void)
{
  sensor_t sensor;
  bno.getSensor(&sensor);
  Serial.println(F("------ Orientation Sensor Details: ------------"));
  Serial.print(F("Sensor:       "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:   "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:    "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:    "));
  Serial.print(sensor.max_value);
  Serial.println(F(" xxx"));
  Serial.print(F("Min Value:    "));
  Serial.print(sensor.min_value);
  Serial.println(F(" xxx"));
  Serial.print(F("Resolution:   "));
  Serial.print(sensor.resolution);
  Serial.println(F(" xxx"));
  Serial.println(F("-------- End Orientation Sensor Details --------"));
  delay(100);
}

/**************************************************************************/
/* Display some basic info about the (orientation) sensor status */
/**************************************************************************/
void orient_displaySensorStatus(void)
{
  /* Get the system status values (mostly for debugging purposes) */
  uint8_t system_status, self_test_results, system_error;
  system_status = self_test_results = system_error = 0;
  bno.getSystemStatus(&system_status, &self_test_results, &system_error);

  /* Display the results in the Serial Monitor */
  Serial.println(F("------ Orientation Sensor Status: ------------"));
  Serial.print(F("System Status: 0x"));
  Serial.println(system_status, HEX);
  Serial.print(F("Self Test:     0x"));
  Serial.println(self_test_results, HEX);
  Serial.print(F("System Error:  0x"));
  Serial.println(system_error, HEX);
  Serial.println(F("------ End Orientation Sensor Status -----------"));
  delay(100);
}

/**************************************************************************/
/* Display sensor calibration status */
/**************************************************************************/
void orient_displayCalibrationStatus(void)
{
  /* Get the four calibration values (0..3) */
  /* Any sensor data reporting 0 should be ignored, */
  /* 3 means 'fully calibrated" */
  uint8_t system, gyro, accel, mag;
  system = gyro = accel = mag = 0;
  bno.getCalibration(&system, &gyro, &accel, &mag);

  Serial.print(F("Orient Calibration: "));

  /* The data should be ignored until the system calibration is > 0 */
  if (not system)
  {
    Serial.print(F("Not Ready! "));
  }
  /* Display the individual values */
  Serial.print(F("Sys:"));
  Serial.print(system, DEC);
  Serial.print(F(" G:"));
  Serial.print(gyro, DEC);
  Serial.print(F(" A:"));
  Serial.print(accel, DEC);
  Serial.print(F(" M:"));
  Serial.print(mag, DEC);
}

void orient_log_all_values()
{
  Serial.print(F("cumulative_yaw_angle:"));
  sd_log_value(datalogFile, sensor_has_locked_on ? orient_get_cumulative_yaw_angle() : NAN);
  Serial.print(F("NS_pitch:"));
  sd_log_value(datalogFile, sensor_has_locked_on ? orient_get_NS_pitch() : NAN);
  Serial.print(F("EW_pitch:"));
  sd_log_value(datalogFile, sensor_has_locked_on ? orient_get_EW_pitch() : NAN);
  Serial.print(F("NS_accel:"));
  sd_log_value(datalogFile, sensor_has_locked_on ? orient_get_NS_accel() : NAN);
  Serial.print(F("EW_accel:"));
  sd_log_value(datalogFile, sensor_has_locked_on ? orient_get_EW_accel() : NAN);
}

#endif
