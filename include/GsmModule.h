#pragma once

#include <Arduino.h>
#include <vector>
#include "config.h"

class GsmModule {
 public:
  bool begin();
  void refreshStatus();
  bool sendSms(const char* number, const String& text);
  bool sendToAll(const String& text, uint8_t* sent_count = nullptr);
  bool ready() const { return ready_; }
  bool simReady() const { return sim_ready_; }
  bool networkRegistered() const { return network_registered_; }
  uint8_t signalBars() const;
  String statusText() const;
  String lastError() const { return last_error_; }
  bool hasPendingNumbers() const { return !failed_numbers_.empty(); }
  bool retryFailed(const String& text, uint8_t* sent_count = nullptr);

 private:
  bool ready_ = false;
  bool sim_ready_ = false;
  bool network_registered_ = false;
  uint8_t signal_quality_ = 99;
  String last_error_;
  std::vector<String> failed_numbers_;
  void sendAt(const String& cmd);
  String readUntil(const char* token, uint32_t timeout_ms);
  bool expectOk(uint32_t timeout_ms);
};
