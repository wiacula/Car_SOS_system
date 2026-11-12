#pragma once

#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "config.h"

class GpsModule {
 public:
  bool begin();
  void poll();
  bool hasFix() const;
  bool timeSynced();
  String timestampText();
  String clockText();
  String diagnostics();
  double lat();
  double lon();
  uint8_t satellites();
  String mapsUrl();
  String coordsText();

 private:
  TinyGPSPlus gps_;
  uint32_t last_rx_ms_ = 0;
  bool timezone_ready_ = false;
};
