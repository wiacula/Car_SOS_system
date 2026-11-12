#include "GsmModule.h"
#include "EmergencyNumbers.h"

static HardwareSerial& gsmSerial = Serial1;

bool GsmModule::begin() {
  ready_ = false;
  sim_ready_ = false;
  network_registered_ = false;
  signal_quality_ = 99;
  pinMode(PIN_GSM_RST, OUTPUT);
  digitalWrite(PIN_GSM_RST, HIGH);
  delay(80);
  digitalWrite(PIN_GSM_RST, LOW);
  delay(120);
  digitalWrite(PIN_GSM_RST, HIGH);
  delay(800);

  gsmSerial.begin(GSM_BAUD, SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
  delay(300);
  sendAt("AT");
  if (!expectOk(1500)) {
    last_error_ = "brak odpowiedzi AT";
    return false;
  }

  sendAt("ATE0");
  expectOk(1000);

  if (SIM_PIN != nullptr && SIM_PIN[0] != '\0') {
    String cmd = "AT+CPIN=\"";
    cmd += SIM_PIN;
    cmd += "\"";
    sendAt(cmd);
    expectOk(3000);
  }

  sendAt("AT+CMGF=1");
  if (!expectOk(1500)) {
    last_error_ = "CMGF";
    return false;
  }

  sendAt("AT+CSCS=\"GSM\"");
  expectOk(1000);

  ready_ = true;
  last_error_ = "";
  return true;
}

void GsmModule::refreshStatus() {
  sim_ready_ = false;
  network_registered_ = false;
  signal_quality_ = 99;
  if (!ready_) {
    return;
  }

  sendAt("AT+CPIN?");
  const String sim_response = readUntil("OK", 300);
  sim_ready_ = sim_response.indexOf("+CPIN: READY") >= 0;

  sendAt("AT+CREG?");
  const String network_response = readUntil("OK", 300);
  const int registration = network_response.indexOf("+CREG:");
  if (registration >= 0) {
    const int comma = network_response.indexOf(',', registration);
    const int value_start =
        comma >= 0 ? comma + 1 : network_response.indexOf(':', registration) + 1;
    const int status = network_response.substring(value_start).toInt();
    network_registered_ = status == 1 || status == 5;
  }

  sendAt("AT+CSQ");
  const String signal_response = readUntil("OK", 300);
  const int signal = signal_response.indexOf("+CSQ:");
  if (signal >= 0) {
    const int quality =
        signal_response.substring(signal + 5).toInt();
    if (quality >= 0 && quality <= 31) {
      signal_quality_ = static_cast<uint8_t>(quality);
    }
  }
}

uint8_t GsmModule::signalBars() const {
  if (!ready_ || !sim_ready_ || !network_registered_ || signal_quality_ == 99) {
    return 0;
  }
  if (signal_quality_ >= 20) {
    return 4;
  }
  if (signal_quality_ >= 14) {
    return 3;
  }
  if (signal_quality_ >= 8) {
    return 2;
  }
  return 1;
}

String GsmModule::statusText() const {
  if (!ready_) {
    return "GSM: BRAK MODULU";
  }
  if (!sim_ready_) {
    return "GSM: BRAK SIM / PIN";
  }
  if (!network_registered_) {
    return "GSM: SZUKA SIECI";
  }
  if (signalBars() == 0) {
    return "GSM: BRAK ZASIEGU";
  }
  return "GSM: SIEC OK";
}

void GsmModule::sendAt(const String& cmd) {
  gsmSerial.print(cmd);
  gsmSerial.print("\r");
}

String GsmModule::readUntil(const char* token, uint32_t timeout_ms) {
  String buf;
  const uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    while (gsmSerial.available()) {
      buf += static_cast<char>(gsmSerial.read());
      if (buf.indexOf(token) >= 0) {
        return buf;
      }
    }
    delay(5);
  }
  return buf;
}

bool GsmModule::expectOk(uint32_t timeout_ms) {
  const String r = readUntil("OK", timeout_ms);
  return r.indexOf("OK") >= 0;
}

bool GsmModule::sendSms(const char* number, const String& text) {
  if (!ready_) {
    last_error_ = "GSM niegotowy";
    return false;
  }

  String cmd = "AT+CMGS=\"";
  cmd += number;
  cmd += "\"";
  sendAt(cmd);

  const String prompt = readUntil(">", 4000);
  if (prompt.indexOf(">") < 0) {
    last_error_ = "brak promptu SMS";
    return false;
  }

  gsmSerial.print(text);
  gsmSerial.write(0x1A);

  const String r = readUntil("OK", 15000);
  if (r.indexOf("OK") < 0 && r.indexOf("+CMGS") < 0) {
    last_error_ = "wysylka SMS";
    return false;
  }
  last_error_ = "";
  return true;
}

bool GsmModule::sendToAll(const String& text, uint8_t* sent_count) {
  if (sent_count != nullptr) {
    *sent_count = 0;
  }
  failed_numbers_.clear();
  std::vector<String> numbers;
  if (!loadEmergencyNumbers(numbers)) {
    last_error_ = "nie mozna odczytac listy numerow SMS";
    return false;
  }
  if (numbers.empty()) {
    last_error_ = "brak numerow SMS";
    return false;
  }

  failed_numbers_ = numbers;
  if (!ready_) {
    last_error_ = "brak modulu GSM";
    return false;
  }
  if (!sim_ready_) {
    last_error_ = "brak gotowej karty SIM lub wymagany PIN";
    return false;
  }
  if (!network_registered_) {
    last_error_ = "brak rejestracji w sieci GSM";
    return false;
  }

  std::vector<String> failed;
  for (size_t i = 0; i < numbers.size(); ++i) {
    if (sendSms(numbers[i].c_str(), text)) {
      if (sent_count != nullptr) {
        ++(*sent_count);
      }
    } else {
      failed.push_back(numbers[i]);
    }
    if (i + 1 < numbers.size()) {
      delay(800);
    }
  }
  failed_numbers_.swap(failed);
  return failed_numbers_.empty();
}

bool GsmModule::retryFailed(const String& text, uint8_t* sent_count) {
  if (sent_count != nullptr) {
    *sent_count = 0;
  }
  if (failed_numbers_.empty()) {
    last_error_ = "brak numerow do ponowienia";
    return false;
  }
  if (!ready_) {
    last_error_ = "brak modulu GSM";
    return false;
  }
  if (!sim_ready_) {
    last_error_ = "brak gotowej karty SIM lub wymagany PIN";
    return false;
  }
  if (!network_registered_) {
    last_error_ = "brak rejestracji w sieci GSM";
    return false;
  }

  std::vector<String> failed;
  for (size_t i = 0; i < failed_numbers_.size(); ++i) {
    if (sendSms(failed_numbers_[i].c_str(), text)) {
      if (sent_count != nullptr) {
        ++(*sent_count);
      }
    } else {
      failed.push_back(failed_numbers_[i]);
    }
    if (i + 1 < failed_numbers_.size()) {
      delay(800);
    }
  }
  failed_numbers_.swap(failed);
  return failed_numbers_.empty();
}
