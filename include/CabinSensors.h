#pragma once

#include "Types.h"
#include "config.h"

class CabinSensors {
 public:
  void begin();
  CrashKind update();
  int gasRaw() const { return gas_raw_; }
  float tempC() const { return temp_c_; }
  float humidity() const { return humidity_; }
  bool tempValid() const { return temp_valid_; }
  bool temperatureReadAttempted() const { return temperature_read_attempted_; }
  bool gasWarmedUp() const {
    return millis() - started_ms_ >= MQ2_WARMUP_MS;
  }

 private:
  int gas_raw_ = 0;
  int gas_hits_ = 0;
  float temp_c_ = 0;
  float humidity_ = 0;
  bool temp_valid_ = false;
  bool temperature_read_attempted_ = false;
  int temp_hits_ = 0;
  uint32_t started_ms_ = 0;
  uint32_t last_dht_ms_ = 0;
};
