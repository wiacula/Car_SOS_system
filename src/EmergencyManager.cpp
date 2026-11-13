#include "EmergencyManager.h"

void EmergencyManager::begin() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_ALARM, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ALARM, LOW);
  state_ = SystemState::Ready;
}

void EmergencyManager::onCrash(CrashKind kind) {
  if (state_ == SystemState::Sending || state_ == SystemState::Sent ||
      state_ == SystemState::RetryWait || state_ == SystemState::Retrying ||
      state_ == SystemState::SendPartial || state_ == SystemState::SendFailed) {
    return;
  }
  if (state_ == SystemState::Countdown) {
    return;
  }
  kind_ = kind;
  state_ = SystemState::Countdown;
  countdown_started_ms_ = millis();
  last_beep_ = countdown_started_ms_;
  countdown_until_ = millis() + COUNTDOWN_MS;
}

void EmergencyManager::onSos() {
  if (state_ == SystemState::Sending || state_ == SystemState::Retrying ||
      state_ == SystemState::Sent) {
    return;
  }
  if (state_ == SystemState::RetryWait) {
    attempts_ = 2;
    state_ = SystemState::Retrying;
    return;
  }
  kind_ = CrashKind::None;
  if (state_ == SystemState::Countdown) {
    countdown_until_ = millis();
    return;
  }
  state_ = SystemState::Countdown;
  countdown_started_ms_ = millis();
  last_beep_ = countdown_started_ms_;
  countdown_until_ = millis() + COUNTDOWN_MS;
}

void EmergencyManager::onCancel() {
  if (state_ == SystemState::Countdown) {
    state_ = SystemState::Ready;
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_ALARM, LOW);
  } else if (state_ == SystemState::RetryWait) {
    resetAfterAck();
  } else if (state_ == SystemState::Sent ||
             state_ == SystemState::SendPartial ||
             state_ == SystemState::SendFailed) {
    resetAfterAck();
  }
}

void EmergencyManager::tick() {
  if (state_ == SystemState::RetryWait) {
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_ALARM, LOW);
    if (static_cast<int32_t>(millis() - retry_until_) >= 0) {
      attempts_ = 2;
      state_ = SystemState::Retrying;
    }
    return;
  }

  if (state_ != SystemState::Countdown) {
    if (state_ != SystemState::Sending && state_ != SystemState::Retrying &&
        state_ != SystemState::Sent && state_ != SystemState::SendPartial &&
        state_ != SystemState::SendFailed) {
      digitalWrite(PIN_LED_ALARM, LOW);
      digitalWrite(PIN_BUZZER, LOW);
    }
    return;
  }

  const uint32_t now = millis();
  const bool alarmLedOn = ((now - countdown_started_ms_) / 400) % 2 == 0;
  digitalWrite(PIN_LED_ALARM, alarmLedOn ? HIGH : LOW);
  if (now - last_beep_ > 400) {
    last_beep_ = now;
    digitalWrite(PIN_BUZZER, !digitalRead(PIN_BUZZER));
  }

  if (static_cast<int32_t>(now - countdown_until_) >= 0) {
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_ALARM, HIGH);
    attempts_ = 1;
    state_ = SystemState::Sending;
  }
}

bool EmergencyManager::alarmActive() const {
  return state_ == SystemState::Countdown || state_ == SystemState::Sending ||
         state_ == SystemState::RetryWait || state_ == SystemState::Retrying ||
         state_ == SystemState::Sent || state_ == SystemState::SendPartial;
}

bool EmergencyManager::shouldSend() const {
  return state_ == SystemState::Sending || state_ == SystemState::Retrying;
}

void EmergencyManager::markAttemptResult(bool success,
                                         bool partial,
                                         bool retryAvailable) {
  if (!success && attempts_ == 1 && retryAvailable) {
    state_ = SystemState::RetryWait;
    retry_until_ = millis() + 3000;
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_ALARM, LOW);
    return;
  }

  state_ = success ? SystemState::Sent
                   : (partial ? SystemState::SendPartial
                              : SystemState::SendFailed);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ALARM, success || partial ? HIGH : LOW);
}

void EmergencyManager::resetAfterAck() {
  state_ = SystemState::Ready;
  kind_ = CrashKind::None;
  attempts_ = 0;
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
