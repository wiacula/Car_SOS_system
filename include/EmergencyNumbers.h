#pragma once

#include <Arduino.h>
#include <vector>

enum class EmergencyNumberResult {
  Success,
  InvalidNumber,
  Duplicate,
  NotFound,
  LimitReached,
  StorageError
};

bool loadEmergencyNumbers(std::vector<String>& numbers);
EmergencyNumberResult addEmergencyNumber(const String& number);
EmergencyNumberResult removeEmergencyNumber(const String& number);
