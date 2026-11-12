#include "EmergencyNumbers.h"

#include <Preferences.h>
#include "config.h"

static const char* NVS_NAMESPACE = "sos";
static const char* NVS_KEY_NUMBERS = "numbers";
static const size_t MAX_EMERGENCY_NUMBERS = 10;

static bool isValidNumber(const String& number) {
  if (number.length() < 8 || number.length() > 16 || number[0] != '+') {
    return false;
  }
  for (size_t i = 1; i < number.length(); ++i) {
    if (number[i] < '0' || number[i] > '9') {
      return false;
    }
  }
  return true;
}

bool loadEmergencyNumbers(std::vector<String>& numbers) {
  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, false)) {
    return false;
  }

  if (!preferences.isKey(NVS_KEY_NUMBERS)) {
    numbers.clear();
    for (int i = 0; i < EMERGENCY_NUMBER_COUNT; ++i) {
      numbers.push_back(EMERGENCY_NUMBERS[i]);
    }
    preferences.end();
    return true;
  }

  const String stored = preferences.getString(NVS_KEY_NUMBERS, "");
  preferences.end();
  numbers.clear();

  int start = 0;
  while (start < stored.length()) {
    const int separator = stored.indexOf(',', start);
    const int end = separator < 0 ? stored.length() : separator;
    const String number = stored.substring(start, end);
    if (!isValidNumber(number) || numbers.size() >= MAX_EMERGENCY_NUMBERS) {
      numbers.clear();
      return false;
    }
    for (const String& existing : numbers) {
      if (existing == number) {
        numbers.clear();
        return false;
      }
    }
    numbers.push_back(number);
    if (separator < 0) {
      break;
    }
    start = separator + 1;
    if (start == stored.length()) {
      numbers.clear();
      return false;
    }
  }
  return true;
}

static bool saveEmergencyNumbers(const std::vector<String>& numbers) {
  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, false)) {
    return false;
  }

  String stored;
  for (size_t i = 0; i < numbers.size(); ++i) {
    if (i > 0) {
      stored += ',';
    }
    stored += numbers[i];
  }
  const size_t written = preferences.putString(NVS_KEY_NUMBERS, stored);
  preferences.end();
  return written > 0;
}

EmergencyNumberResult addEmergencyNumber(const String& number) {
  if (!isValidNumber(number)) {
    return EmergencyNumberResult::InvalidNumber;
  }

  std::vector<String> numbers;
  if (!loadEmergencyNumbers(numbers)) {
    return EmergencyNumberResult::StorageError;
  }
  for (const String& existing : numbers) {
    if (existing == number) {
      return EmergencyNumberResult::Duplicate;
    }
  }
  if (numbers.size() >= MAX_EMERGENCY_NUMBERS) {
    return EmergencyNumberResult::LimitReached;
  }
  numbers.push_back(number);
  return saveEmergencyNumbers(numbers) ? EmergencyNumberResult::Success
                                       : EmergencyNumberResult::StorageError;
}

EmergencyNumberResult removeEmergencyNumber(const String& number) {
  std::vector<String> numbers;
  if (!loadEmergencyNumbers(numbers)) {
    return EmergencyNumberResult::StorageError;
  }

  for (size_t i = 0; i < numbers.size(); ++i) {
    if (numbers[i] == number) {
      numbers.erase(numbers.begin() + i);
      return saveEmergencyNumbers(numbers) ? EmergencyNumberResult::Success
                                           : EmergencyNumberResult::StorageError;
    }
  }
  return EmergencyNumberResult::NotFound;
}
