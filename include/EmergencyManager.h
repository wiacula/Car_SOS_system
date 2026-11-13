#pragma once

#include <Arduino.h>
#include "Types.h"
#include "config.h"

class EmergencyManager {
 public:
  void begin();
  void onCrash(CrashKind kind);
  void onSos();
  void onCancel();
  void tick();

  bool alarmActive() const;
  bool shouldSend() const;
  uint8_t attemptNumber() const { return attempts_; }
  void markAttemptResult(bool success, bool partial, bool retryAvailable);
  void resetAfterAck();

  CrashKind kind() const { return kind_; }
  int countdownSeconds() const;
  SystemState state() const;

 private:
  SystemState state_ = SystemState::Ready;
  CrashKind kind_ = CrashKind::None;
  uint32_t countdown_until_ = 0;
  uint32_t countdown_started_ms_ = 0;
  uint32_t retry_until_ = 0;
  uint32_t last_beep_ = 0;
  uint8_t attempts_ = 0;
};
