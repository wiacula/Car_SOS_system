#pragma once

#include <Arduino.h>
#include "config.h"

class GsmModule {
 public:
  bool begin();
  bool sendSms(const char* number, const String& text);
  bool sendToAll(const String& text);
  bool ready() const { return ready_; }
  String lastError() const { return last_error_; }

 private:
  bool ready_ = false;
  String last_error_;
  void sendAt(const String& cmd);
  String readUntil(const char* token, uint32_t timeout_ms);
  bool expectOk(uint32_t timeout_ms);
};
