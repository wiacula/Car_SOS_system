#include "CrashDetector.h"
#include <cmath>

static float normalizeAngle(float angle) {
  while (angle > 180.0f) {
    angle -= 360.0f;
  }
  while (angle < -180.0f) {
    angle += 360.0f;
  }
  return angle;
}

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

bool CrashDetector::calibrate(void (*progressCallback)(uint8_t)) {
  static const uint16_t CALIBRATION_SAMPLES = 100;
  static const float GRAVITY_M_S2 = 9.80665f;

  Serial.println("Kalibracja MPU-6050: nie ruszaj urzadzeniem...");
  float ax_sum = 0;
  float ay_sum = 0;
  float az_sum = 0;
  float gx_sum = 0;
  float gy_sum = 0;
  float gz_sum = 0;
  sensors_event_t a, g, temp;
  for (uint16_t i = 0; i < CALIBRATION_SAMPLES; ++i) {
    mpu_.getEvent(&a, &g, &temp);
    ax_sum += a.acceleration.x;
    ay_sum += a.acceleration.y;
    az_sum += a.acceleration.z;
    gx_sum += g.gyro.x;
    gy_sum += g.gyro.y;
    gz_sum += g.gyro.z;
    delay(10);
    if (progressCallback &&
        ((i + 1) % 10 == 0 || i + 1 == CALIBRATION_SAMPLES)) {
      progressCallback((i + 1) * 100 / CALIBRATION_SAMPLES);
    }
  }

  const float sample_count = static_cast<float>(CALIBRATION_SAMPLES);
  const float ax_avg = ax_sum / sample_count;
  const float ay_avg = ay_sum / sample_count;
  const float az_avg = az_sum / sample_count;
  const float gravity = sqrtf(ax_avg * ax_avg + ay_avg * ay_avg + az_avg * az_avg);
  if (!isfinite(gravity) || gravity < 0.5f * GRAVITY_M_S2) {
    ready_ = false;
    Serial.println("Blad kalibracji MPU-6050: nieprawidlowy odczyt akcelerometru");
    return false;
  }

  const float gravity_scale = GRAVITY_M_S2 / gravity;
  accel_offset_x_ = ax_avg - ax_avg * gravity_scale;
  accel_offset_y_ = ay_avg - ay_avg * gravity_scale;
  accel_offset_z_ = az_avg - az_avg * gravity_scale;
  gyro_bias_x_ = gx_sum / sample_count;
  gyro_bias_y_ = gy_sum / sample_count;
  gyro_bias_z_ = gz_sum / sample_count;

  const float ax = (ax_avg - accel_offset_x_) / GRAVITY_M_S2;
  const float ay = (ay_avg - accel_offset_y_) / GRAVITY_M_S2;
  const float az = (az_avg - accel_offset_z_) / GRAVITY_M_S2;
  accel_x_g_ = ax;
  accel_y_g_ = ay;
  accel_z_g_ = az;
  gyro_x_dps_ = 0;
  gyro_y_dps_ = 0;
  gyro_z_dps_ = 0;
  base_roll_deg_ = atan2f(ay, az) * 180.0f / PI;
  base_pitch_deg_ = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / PI;
  roll_deg_ = 0;
  pitch_deg_ = 0;
  yaw_deg_ = 0;
  accel_g_ = 1.0f;
  last_us_ = micros();
  Serial.println("Kalibracja MPU-6050 zakonczona");
  return true;
}

CrashKind CrashDetector::update() {
  if (!ready_) {
    return CrashKind::None;
  }

  sensors_event_t a, g, temp;
  mpu_.getEvent(&a, &g, &temp);

  const float ax = (a.acceleration.x - accel_offset_x_) / 9.80665f;
  const float ay = (a.acceleration.y - accel_offset_y_) / 9.80665f;
  const float az = (a.acceleration.z - accel_offset_z_) / 9.80665f;
  accel_x_g_ = ax;
  accel_y_g_ = ay;
  accel_z_g_ = az;
  accel_g_ = sqrtf(ax * ax + ay * ay + az * az);

  const uint32_t now_us = micros();
  float dt = (now_us - last_us_) / 1e6f;
  last_us_ = now_us;
  if (dt <= 0 || dt > 0.1f) {
    dt = SAMPLE_MS / 1000.0f;
  }

  const float gyro_x_dps = (g.gyro.x - gyro_bias_x_) * 180.0f / PI;
  const float gyro_y_dps = (g.gyro.y - gyro_bias_y_) * 180.0f / PI;
  const float gyro_z_dps = (g.gyro.z - gyro_bias_z_) * 180.0f / PI;
  gyro_x_dps_ = gyro_x_dps;
  gyro_y_dps_ = gyro_y_dps;
  gyro_z_dps_ = gyro_z_dps;

  const float accel_roll = normalizeAngle(
      atan2f(ay, az) * 180.0f / PI - base_roll_deg_);
  const float accel_pitch = normalizeAngle(
      atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / PI -
      base_pitch_deg_);

  roll_deg_ = COMP_FILTER_ALPHA * (roll_deg_ + gyro_x_dps * dt) +
              (1.0f - COMP_FILTER_ALPHA) * accel_roll;
  pitch_deg_ = COMP_FILTER_ALPHA * (pitch_deg_ + gyro_y_dps * dt) +
               (1.0f - COMP_FILTER_ALPHA) * accel_pitch;
  yaw_deg_ = normalizeAngle(yaw_deg_ + gyro_z_dps * dt);

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
