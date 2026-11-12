#pragma once

#include <Arduino.h>

enum class CrashKind : uint8_t { None, Impact, Rollover, Gas, Temperature };

enum class SystemState : uint8_t {
  Boot,
  Ready,
  Countdown,
  Sending,
  RetryWait,
  Retrying,
  Sent,
  SendPartial,
  SendFailed,
  Fault
};

inline const char* crashKindName(CrashKind k) {
  switch (k) {
    case CrashKind::Impact:
      return "UDERZENIE";
    case CrashKind::Rollover:
      return "DACHOWANIE";
    case CrashKind::Gas:
      return "GAZ";
    case CrashKind::Temperature:
      return "TEMPERATURA";
    case CrashKind::None:
    default:
      return "SOS";
  }
}
