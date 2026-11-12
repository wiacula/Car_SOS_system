#include "CabinSensors.h"

#include <DHT.h>

static DHT dht(PIN_DHT, DHT11);

void CabinSensors::begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_MQ2, ADC_11db);
  dht.begin();
  started_ms_ = millis();
  last_dht_ms_ = started_ms_;
}

CrashKind CabinSensors::update() {
  gas_raw_ = analogRead(PIN_MQ2);

  const uint32_t now = millis();
  bool new_sample = false;
  if (now - last_dht_ms_ >= DHT_PERIOD_MS) {
    last_dht_ms_ = now;
    const float t = dht.readTemperature();
    const float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temp_c_ = t + TEMP_CALIBRATION_OFFSET_C;
      humidity_ = h;
      temp_valid_ = true;
      new_sample = true;
    }
  }

  if (now - started_ms_ < STARTUP_IGNORE_MS) {
    gas_hits_ = 0;
    temp_hits_ = 0;
    return CrashKind::None;
  }

  if (new_sample) {
    if (temp_c_ >= TEMP_ALARM_C) {
      temp_hits_++;
      if (temp_hits_ >= TEMP_HIT_COUNT) {
        temp_hits_ = 0;
        return CrashKind::Temperature;
      }
    } else {
      temp_hits_ = 0;
    }
  }

  if (now - started_ms_ < MQ2_WARMUP_MS) {
    gas_hits_ = 0;
    return CrashKind::None;
  }

  if (gas_raw_ >= MQ2_ADC_THRESHOLD) {
    gas_hits_++;
    if (gas_hits_ >= MQ2_HIT_COUNT) {
      gas_hits_ = 0;
      return CrashKind::Gas;
    }
  } else {
    gas_hits_ = 0;
  }

  return CrashKind::None;
}
