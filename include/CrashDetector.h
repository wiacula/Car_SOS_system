#pragma once

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "Types.h"
#include "config.h"

class CrashDetector {
 public:
  bool begin();
  void calibrate();
  CrashKind update();
  float accelG() const { return accel_g_; }
  float rollDeg() const { return roll_deg_; }
  float pitchDeg() const { return pitch_deg_; }
  bool ready() const { return ready_; }

 private:
  Adafruit_MPU6050 mpu_;
  bool ready_ = false;
  float accel_g_ = 1.0f;
  float roll_deg_ = 0;
  float pitch_deg_ = 0;
  int hit_streak_ = 0;
  uint32_t rollover_since_ = 0;
  uint32_t last_us_ = 0;
  uint32_t started_ms_ = 0;
};
