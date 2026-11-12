#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include "Types.h"
#include "config.h"

class DisplayUi {
 public:
  bool begin();
  void show(SystemState state,
            float g,
            float roll,
            float pitch,
            const String& gpsLine,
            const std::vector<String>& smsNumbers,
            bool smsNumbersAvailable,
            int countdown_s,
            const String& extra);

 private:
  Adafruit_SSD1306 oled_{OLED_WIDTH, OLED_HEIGHT, &Wire, -1};
  bool ready_ = false;
};
