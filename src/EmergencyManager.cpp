#include "EmergencyManager.h"

void EmergencyManager::begin() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_ALARM, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ALARM, LOW);
  state_ = SystemState::Ready;
}

void EmergencyManager::onCrash(CrashKind kind) {
  if (state_ == SystemState::Sending || state_ == SystemState::Sent) {
    return;
  }
  if (state_ == SystemState::Countdown) {
    return;
  }
  kind_ = kind;
  state_ = SystemState::Countdown;
  countdown_until_ = millis() + COUNTDOWN_MS;
}

void EmergencyManager::onSos() {
  if (state_ == SystemState::Sending || state_ == SystemState::Sent) {
    return;
  }
  kind_ = CrashKind::None;
  if (state_ == SystemState::Countdown) {
    countdown_until_ = millis();
    return;
  }
  state_ = SystemState::Countdown;
  countdown_until_ = millis() + COUNTDOWN_MS;
}

void EmergencyManager::onCancel() {
  if (state_ == SystemState::Countdown) {
    state_ = SystemState::Ready;
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_ALARM, LOW);
  } else if (state_ == SystemState::Sent) {
    resetAfterAck();
  }
}

void EmergencyManager::tick() {
  if (state_ != SystemState::Countdown) {
    if (state_ != SystemState::Sending && state_ != SystemState::Sent) {
      digitalWrite(PIN_LED_ALARM, LOW);
      digitalWrite(PIN_BUZZER, LOW);
    }
    return;
  }

  digitalWrite(PIN_LED_ALARM, HIGH);
  const uint32_t now = millis();
  if (now - last_beep_ > 400) {
    last_beep_ = now;
    digitalWrite(PIN_BUZZER, !digitalRead(PIN_BUZZER));
  }

  if (static_cast<int32_t>(now - countdown_until_) >= 0) {
    digitalWrite(PIN_BUZZER, HIGH);
    state_ = SystemState::Sending;
  }
}

bool EmergencyManager::alarmActive() const {
  return state_ == SystemState::Countdown || state_ == SystemState::Sending ||
         state_ == SystemState::Sent;
}

bool EmergencyManager::shouldSend() const {
  return state_ == SystemState::Sending;
}

void EmergencyManager::markSent() {
  state_ = SystemState::Sent;
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ALARM, HIGH);
}

void EmergencyManager::resetAfterAck() {
  state_ = SystemState::Ready;
  kind_ = CrashKind::None;
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ALARM, LOW);
}

int EmergencyManager::countdownSeconds() const {
  if (state_ != SystemState::Countdown) {
    return 0;
  }
  const int32_t left = static_cast<int32_t>(countdown_until_ - millis());
  if (left <= 0) {
    return 0;
  }
  return (left + 999) / 1000;
}

SystemState EmergencyManager::state() const { return state_; }
