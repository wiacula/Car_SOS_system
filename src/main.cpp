#include <Arduino.h>
#include <Wire.h>

#include "CabinSensors.h"
#include "CrashDetector.h"
#include "DemoWeb.h"
#include "DisplayUi.h"
#include "EmergencyManager.h"
#include "EmergencyNumbers.h"
#include "GpsModule.h"
#include "GsmModule.h"
#include "config.h"

#include <vector>

CrashDetector crash;
CabinSensors cabin;
GpsModule gps;
GsmModule gsm;
DisplayUi ui;
EmergencyManager emergency;

volatile bool sos_web = false;
volatile bool cancel_web = false;

void demoRequestSos() { sos_web = true; }
void demoRequestCancel() { cancel_web = true; }

static uint32_t last_sample = 0;
static uint32_t last_ui = 0;
static uint32_t last_contacts_refresh = 0;
static bool imu_ok = false;
static bool gsm_ok = false;
static bool sms_numbers_available = false;
static std::vector<String> sms_numbers;

static bool buttonPressed(int pin, uint32_t* last_ms) {
  if (digitalRead(pin) != LOW) {
    return false;
  }
  const uint32_t now = millis();
  if (now - *last_ms < DEBOUNCE_MS) {
    return false;
  }
  delay(DEBOUNCE_MS);
  if (digitalRead(pin) != LOW) {
    return false;
  }
  *last_ms = now;
  while (digitalRead(pin) == LOW) {
    delay(5);
  }
  return true;
}

static String buildSms(CrashKind kind) {
  String msg;
  msg += "ALARM ";
  msg += VEHICLE_ID;
  msg += "\nTyp: ";
  msg += crashKindName(kind);
  msg += "\n";
  msg += gps.coordsText();
  msg += "\n";
  const String url = gps.mapsUrl();
  if (url.length() > 0) {
    msg += url;
  } else {
    msg += "GPS: BRAK FIX";
  }
  if (cabin.tempValid()) {
    msg += "\nKabina: ";
    msg += String(cabin.tempC(), 0);
    msg += "C ";
    msg += String(cabin.humidity(), 0);
    msg += "%";
  }
  return msg;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("System ratunkowy — start");

  pinMode(PIN_BTN_SOS, INPUT_PULLUP);
  pinMode(PIN_BTN_CANCEL, INPUT_PULLUP);
  pinMode(PIN_LED_OK, OUTPUT);
  digitalWrite(PIN_LED_OK, LOW);

  Wire.begin(PIN_SDA, PIN_SCL);

  ui.begin();
  sms_numbers_available = loadEmergencyNumbers(sms_numbers);
  if (!sms_numbers_available) {
    Serial.println("Nie mozna odczytac listy numerow SMS");
  }
  last_contacts_refresh = millis();
  imu_ok = crash.begin();
  if (imu_ok) {
    crash.calibrate();
  }
  cabin.begin();
  gps.begin();
  gsm_ok = gsm.begin();
  emergency.begin();
  demoWebBegin();

  if (!imu_ok) {
    Serial.println("MPU-6050 niedostepny");
  }
  if (!gsm_ok) {
    Serial.println("GSM niedostepny — tryb DEMO / log Serial");
  }
  digitalWrite(PIN_LED_OK, imu_ok ? HIGH : LOW);
}

void loop() {
  gps.poll();
  demoWebLoop();
  emergency.tick();

  static uint32_t last_sos_btn = 0;
  static uint32_t last_cancel_btn = 0;

  if (buttonPressed(PIN_BTN_SOS, &last_sos_btn) || sos_web) {
    sos_web = false;
    emergency.onSos();
    Serial.println("SOS reczny");
  }
  if (buttonPressed(PIN_BTN_CANCEL, &last_cancel_btn) || cancel_web) {
    cancel_web = false;
    emergency.onCancel();
    Serial.println("Anulowano / reset");
  }

  const uint32_t now = millis();
  if (now - last_contacts_refresh >= 1000) {
    last_contacts_refresh = now;
    std::vector<String> updated_numbers;
    const bool loaded = loadEmergencyNumbers(updated_numbers);
    if (loaded) {
      sms_numbers.swap(updated_numbers);
    } else {
      sms_numbers.clear();
      if (sms_numbers_available) {
        Serial.println("Nie mozna odczytac listy numerow SMS");
      }
    }
    sms_numbers_available = loaded;
  }

  if (now - last_sample >= SAMPLE_MS) {
    last_sample = now;
    CrashKind k = CrashKind::None;
    if (imu_ok) {
      k = crash.update();
    }
    if (k == CrashKind::None) {
      k = cabin.update();
    }
    if (k != CrashKind::None) {
      Serial.print("Detekcja: ");
      Serial.println(crashKindName(k));
      emergency.onCrash(k);
    }
  }

  String extra_log;
  if (emergency.shouldSend()) {
    const String sms = buildSms(emergency.kind());
    Serial.println("=== TRESC ALARMU ===");
    Serial.println(sms);
    bool ok = false;
    if (gsm_ok) {
      ok = gsm.sendToAll(sms);
    }
    extra_log = ok ? "SMS wyslany" : "SMS DEMO (brak GSM lub blad)";
    if (!ok && gsm_ok) {
      extra_log += " ";
      extra_log += gsm.lastError();
    }
    emergency.markSent();
    demoWebPush(emergency.state(), emergency.kind(), crash.accelG(),
                crash.rollDeg(), crash.pitchDeg(), gps.coordsText(),
                gps.diagnostics(), cabin.tempC(), cabin.tempValid(),
                0, extra_log);
  }

  if (now - last_ui >= 250) {
    last_ui = now;
    SystemState st = emergency.state();
    if (!imu_ok && st == SystemState::Ready) {
      st = SystemState::Fault;
    }
    String extra = gsm_ok ? "GSM " : "DEMO ";
    extra += "G";
    extra += cabin.gasRaw();
    extra += " T";
    if (cabin.tempValid()) {
      extra += String(cabin.tempC(), 0);
    } else {
      extra += "--";
    }
    ui.show(st, crash.accelG(), crash.rollDeg(), crash.pitchDeg(),
            gps.coordsText(), sms_numbers, sms_numbers_available,
            emergency.countdownSeconds(), extra);
    demoWebPush(st, emergency.kind(), crash.accelG(), crash.rollDeg(),
                crash.pitchDeg(), gps.coordsText(), gps.diagnostics(),
                cabin.tempC(), cabin.tempValid(),
                emergency.countdownSeconds(), "");
    digitalWrite(PIN_LED_OK, gps.hasFix() ? HIGH : ((now / 400) % 2));
  }
}
