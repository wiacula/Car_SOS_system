#include "GsmModule.h"
#include "EmergencyNumbers.h"

static HardwareSerial& gsmSerial = Serial1;

static bool probeAt(uint32_t baud, uint8_t attempts) {
  gsmSerial.end();
  gsmSerial.begin(baud, SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
  delay(300);

  for (uint8_t attempt = 0; attempt < attempts; ++attempt) {
    while (gsmSerial.available() > 0) {
      gsmSerial.read();
    }
    gsmSerial.print("AT\r");
    const uint32_t start = millis();
    String response;
    while (millis() - start < 500) {
      while (gsmSerial.available() > 0) {
        response += static_cast<char>(gsmSerial.read());
      }
      if (response.indexOf("OK") >= 0) {
        Serial.print("Modem GSM odpowiada na AT, baud ");
        Serial.println(baud);
        return true;
      }
      delay(5);
    }
    delay(100);
  }
  return false;
}

bool GsmModule::begin() {
  ready_ = false;
  sim_ready_ = false;
  sim_pin_required_ = false;
  sim_puk_required_ = false;
  sim_absent_ = false;
  sms_text_mode_ready_ = false;
  sim_response_ = "";
  network_registered_ = false;
  signal_quality_ = 99;
  last_error_ = "";
  pinMode(PIN_GSM_RST, OUTPUT);
  digitalWrite(PIN_GSM_RST, HIGH);
  delay(80);
  digitalWrite(PIN_GSM_RST, LOW);
  delay(120);
  digitalWrite(PIN_GSM_RST, HIGH);
  delay(800);

  static const uint32_t baud_rates[] = {GSM_BAUD, 115200, 57600,
                                        38400,    19200, 4800};
  bool at_responded = probeAt(baud_rates[0], 5);
  for (size_t i = 1;
       !at_responded && i < sizeof(baud_rates) / sizeof(baud_rates[0]); ++i) {
    at_responded = probeAt(baud_rates[i], 2);
  }
  if (!at_responded) {
    last_error_ = "brak odpowiedzi AT";
    Serial.println(
        "Modem GSM nie odpowiedzial na AT przy sprawdzonych predkosciach");
    return false;
  }

  ready_ = true;
  sendAt("ATE0");
  if (!expectOk(1000)) {
    Serial.println("Modem odpowiada na AT, ale nie potwierdzil ATE0");
  }

  if (SIM_PIN != nullptr && SIM_PIN[0] != '\0') {
    String cmd = "AT+CPIN=\"";
    cmd += SIM_PIN;
    cmd += "\"";
    sendAt(cmd);
    expectOk(3000);
  }

  if (!setSmsTextMode()) {
    Serial.print("Modem odpowiada, konfiguracja SMS nieudana: ");
    Serial.println(last_error_);
  }

  sendAt("AT+CSCS=\"GSM\"");
  if (!expectOk(1000)) {
    Serial.println("Nie udalo sie ustawic kodowania GSM dla SMS");
  }

  return true;
}

void GsmModule::refreshStatus() {
  sim_ready_ = false;
  network_registered_ = false;
  signal_quality_ = 99;
  if (!ready_) {
    return;
  }

  sim_pin_required_ = false;
  sim_puk_required_ = false;
  sim_absent_ = false;
  sendAt("AT+CPIN?");
  const String raw_sim_response = readAtResponse(1000);
  Serial.print("Odpowiedz AT+CPIN?: ");
  Serial.println(raw_sim_response);
  sim_response_ = "";
  for (size_t i = 0; i < raw_sim_response.length(); ++i) {
    const uint8_t character = raw_sim_response[i];
    if (character >= 0x20 && character <= 0x7e) {
      sim_response_ += static_cast<char>(character);
    }
  }
  sim_response_.trim();
  String normalized_sim_response = sim_response_;
  normalized_sim_response.toLowerCase();
  sim_ready_ = normalized_sim_response.indexOf("+cpin: ready") >= 0;
  sim_pin_required_ = normalized_sim_response.indexOf("sim pin") >= 0;
  sim_puk_required_ = normalized_sim_response.indexOf("sim puk") >= 0;
  sim_absent_ = normalized_sim_response.indexOf("not inserted") >= 0 ||
                normalized_sim_response.indexOf("no sim") >= 0 ||
                normalized_sim_response.indexOf("+cpin: absent") >= 0;

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

String GsmModule::simStatusText() const {
  if (!ready_) {
    return "BRAK AT";
  }
  if (sim_ready_) {
    return "GOTOWA";
  }
  if (sim_absent_) {
    return "BRAK KARTY SIM";
  }
  if (sim_pin_required_) {
    return "WYMAGA PIN";
  }
  if (sim_puk_required_) {
    return "WYMAGA PUK";
  }
  if (sim_response_.indexOf("ERROR") >= 0) {
    return "BRAK SIM";
  }
  if (sim_response_.length() == 0) {
    return "BRAK ODPOWIEDZI";
  }
  return "SIM NIEGOTOWA";
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
    return "GSM: BRAK ODPOWIEDZI";
  }
  if (!sim_ready_) {
    String status = "GSM: ";
    status += simStatusText();
    return status;
  }
  if (!network_registered_) {
    return "GSM: SZUKA SIECI";
  }
  if (signalBars() == 0) {
    return "GSM: BRAK ZASIEGU";
  }
  if (!sms_text_mode_ready_) {
    return "GSM: BLAD KONFIG SMS";
  }
  return "GSM: SIEC OK";
}

bool GsmModule::setSmsTextMode() {
  sendAt("AT+CMGF=1");
  const String response = readAtResponse(1500);
  sms_text_mode_ready_ = response.indexOf("OK") >= 0;
  if (!sms_text_mode_ready_) {
    last_error_ = "AT+CMGF=1";
    Serial.print("Odpowiedz AT+CMGF=1: ");
    Serial.println(response);
    return false;
  }
  last_error_ = "";
  return true;
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

String GsmModule::readAtResponse(uint32_t timeout_ms) {
  String response;
  const uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    while (gsmSerial.available() > 0) {
      response += static_cast<char>(gsmSerial.read());
      if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0) {
        return response;
      }
    }
    delay(5);
  }
  return response;
}

bool GsmModule::expectOk(uint32_t timeout_ms) {
  const String r = readAtResponse(timeout_ms);
  return r.indexOf("OK") >= 0;
}

bool GsmModule::sendSms(const char* number, const String& text) {
  if (!ready_) {
    last_error_ = "brak odpowiedzi modemu AT";
    return false;
  }
  if (!sms_text_mode_ready_ && !setSmsTextMode()) {
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
    last_error_ = "brak odpowiedzi AT modemu";
    return false;
  }
  if (!sim_ready_) {
    last_error_ = simStatusText();
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
    last_error_ = "brak odpowiedzi AT modemu";
    return false;
  }
  if (!sim_ready_) {
    last_error_ = simStatusText();
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
