#pragma once

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "Types.h"
#include "config.h"

class CrashDetector {
 public:
  bool begin();
  bool calibrate(void (*progressCallback)(uint8_t) = nullptr);
  CrashKind update();
  float accelG() const { return accel_g_; }
  float accelXG() const { return accel_x_g_; }
  float accelYG() const { return accel_y_g_; }
  float accelZG() const { return accel_z_g_; }
  float gyroXDps() const { return gyro_x_dps_; }
  float gyroYDps() const { return gyro_y_dps_; }
  float gyroZDps() const { return gyro_z_dps_; }
  float rollDeg() const { return roll_deg_; }
  float pitchDeg() const { return pitch_deg_; }
  float yawDeg() const { return yaw_deg_; }
  bool ready() const { return ready_; }

 private:
  Adafruit_MPU6050 mpu_;
  bool ready_ = false;
  float accel_g_ = 1.0f;
  float accel_x_g_ = 0;
  float accel_y_g_ = 0;
  float accel_z_g_ = 1;
  float gyro_x_dps_ = 0;
  float gyro_y_dps_ = 0;
  float gyro_z_dps_ = 0;
  float roll_deg_ = 0;
  float pitch_deg_ = 0;
  float yaw_deg_ = 0;
  float base_roll_deg_ = 0;
  float base_pitch_deg_ = 0;
  float accel_offset_x_ = 0;
  float accel_offset_y_ = 0;
  float accel_offset_z_ = 0;
  float gyro_bias_x_ = 0;
  float gyro_bias_y_ = 0;
  float gyro_bias_z_ = 0;
  int hit_streak_ = 0;
  uint32_t rollover_since_ = 0;
  uint32_t last_us_ = 0;
  uint32_t started_ms_ = 0;
};
