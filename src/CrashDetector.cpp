#include "CrashDetector.h"
#include <cmath>

bool CrashDetector::begin() {
  if (!mpu_.begin()) {
    return false;
  }
  mpu_.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu_.setGyroRange(MPU6050_RANGE_2000_DEG);
  mpu_.setFilterBandwidth(MPU6050_BAND_21_HZ);
  started_ms_ = millis();
  last_us_ = micros();
  ready_ = true;
  return true;
}

void CrashDetector::calibrate() {
  // Krótka pauza — filtr IIR MPU i ustalenie grawitacji.
  delay(200);
  sensors_event_t a, g, temp;
  mpu_.getEvent(&a, &g, &temp);
  const float ax = a.acceleration.x / 9.80665f;
  const float ay = a.acceleration.y / 9.80665f;
  const float az = a.acceleration.z / 9.80665f;
  roll_deg_ = atan2f(ay, az) * 180.0f / PI;
  pitch_deg_ = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / PI;
  last_us_ = micros();
}

CrashKind CrashDetector::update() {
  if (!ready_) {
    return CrashKind::None;
  }

  sensors_event_t a, g, temp;
  mpu_.getEvent(&a, &g, &temp);

  const float ax = a.acceleration.x / 9.80665f;
  const float ay = a.acceleration.y / 9.80665f;
  const float az = a.acceleration.z / 9.80665f;
  accel_g_ = sqrtf(ax * ax + ay * ay + az * az);

  const uint32_t now_us = micros();
  float dt = (now_us - last_us_) / 1e6f;
  last_us_ = now_us;
  if (dt <= 0 || dt > 0.1f) {
    dt = SAMPLE_MS / 1000.0f;
  }

  const float gyro_x_dps = g.gyro.x * 180.0f / PI;
  const float gyro_y_dps = g.gyro.y * 180.0f / PI;

  const float accel_roll = atan2f(ay, az) * 180.0f / PI;
  const float accel_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / PI;

  roll_deg_ = COMP_FILTER_ALPHA * (roll_deg_ + gyro_x_dps * dt) +
              (1.0f - COMP_FILTER_ALPHA) * accel_roll;
  pitch_deg_ = COMP_FILTER_ALPHA * (pitch_deg_ + gyro_y_dps * dt) +
               (1.0f - COMP_FILTER_ALPHA) * accel_pitch;

  if (millis() - started_ms_ < STARTUP_IGNORE_MS) {
    hit_streak_ = 0;
    rollover_since_ = 0;
    return CrashKind::None;
  }

  if (accel_g_ >= CRASH_G_THRESHOLD) {
    hit_streak_++;
    if (hit_streak_ >= CRASH_HIT_COUNT) {
      hit_streak_ = 0;
      return CrashKind::Impact;
    }
  } else {
    hit_streak_ = 0;
  }

  const bool tilted = (fabsf(roll_deg_) >= ROLLOVER_DEG) ||
                      (fabsf(pitch_deg_) >= ROLLOVER_DEG);
  if (tilted) {
    if (rollover_since_ == 0) {
      rollover_since_ = millis();
    } else if (millis() - rollover_since_ >= ROLLOVER_HOLD_MS) {
      rollover_since_ = 0;
      return CrashKind::Rollover;
    }
  } else {
    rollover_since_ = 0;
  }

  return CrashKind::None;
}
