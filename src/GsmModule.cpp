#include "GsmModule.h"
#include "EmergencyNumbers.h"

static HardwareSerial& gsmSerial = Serial1;

bool GsmModule::begin() {
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
  return true;
}

bool GsmModule::sendToAll(const String& text) {
  std::vector<String> numbers;
  if (!loadEmergencyNumbers(numbers)) {
    last_error_ = "nie mozna odczytac listy numerow SMS";
    return false;
  }
  if (numbers.empty()) {
    last_error_ = "brak numerow SMS";
    return false;
  }

  bool any = false;
  for (size_t i = 0; i < numbers.size(); ++i) {
    if (sendSms(numbers[i].c_str(), text)) {
      any = true;
    }
    if (i + 1 < numbers.size()) {
      delay(800);
    }
  }
  return any;
}
