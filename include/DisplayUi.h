#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include "Types.h"
#include "config.h"

struct DisplayMenuData {
  bool imuReady = false;
  float accelXG = 0;
  float accelYG = 0;
  float accelZG = 0;
  float gyroXDps = 0;
  float gyroYDps = 0;
  float gyroZDps = 0;
  int gasRaw = 0;
  float temperatureC = 0;
  bool temperatureValid = false;
  String gpsDiagnostics;
  bool gpsFix = false;
  bool gsmReady = false;
  bool simReady = false;
  bool networkRegistered = false;
};

class DisplayUi {
 public:
  bool begin();
  void showStartupAnimation(uint8_t frame);
  void showCalibrationMessage(uint8_t progress, bool sampling);
  void show(SystemState state,
            float g,
            float roll,
            float pitch,
            float yaw,
            const String& gpsLine,
            uint8_t gpsSatellites,
            uint8_t gsmSignalBars,
            const String& gsmStatus,
            const String& clockText,
            const std::vector<String>& smsNumbers,
            bool smsNumbersAvailable,
            int countdown_s,
            uint8_t menuPage,
            const DisplayMenuData& menuData);

 private:
  void showMenu(uint8_t page,
                float g,
                const String& gpsLine,
                uint8_t gpsSatellites,
                uint8_t gsmSignalBars,
                const String& gsmStatus,
                const std::vector<String>& smsNumbers,
                bool smsNumbersAvailable,
                const DisplayMenuData& menuData);

  Adafruit_SSD1306 oled_{OLED_WIDTH, OLED_HEIGHT, &Wire, -1};
  bool ready_ = false;
};
