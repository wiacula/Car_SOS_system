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
static uint32_t last_gsm_refresh = 0;
static uint8_t sms_sent_total = 0;
static uint8_t menu_page = 0;
static bool imu_ok = false;
static bool gsm_ok = false;
static bool sms_numbers_available = false;
static std::vector<String> sms_numbers;
static String last_gsm_status;
static bool gps_was_fixed = false;

static void logWebEvent(const String& message) {
  demoWebSetClock(gps.timestampText(), gps.timeSynced());
  demoWebLog(message);
}

static void logGsmStatusIfChanged() {
  const String status = gsm.statusText();
  if (status != last_gsm_status) {
    last_gsm_status = status;
    logWebEvent(status);
  }
}

static void showCalibrationProgress(uint8_t progress) {
  ui.showCalibrationMessage(progress, true);
}

static DisplayMenuData getDisplayMenuData() {
  DisplayMenuData data;
  data.imuReady = imu_ok;
  data.accelXG = crash.accelXG();
  data.accelYG = crash.accelYG();
  data.accelZG = crash.accelZG();
  data.gyroXDps = crash.gyroXDps();
  data.gyroYDps = crash.gyroYDps();
  data.gyroZDps = crash.gyroZDps();
  data.gasRaw = cabin.gasRaw();
  data.temperatureC = cabin.tempC();
  data.temperatureValid = cabin.tempValid();
  data.gpsDiagnostics = gps.diagnostics();
  data.gpsFix = gps.hasFix();
  data.gsmReady = gsm.ready();
  data.simReady = gsm.simReady();
  data.networkRegistered = gsm.networkRegistered();
  return data;
}

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
  logWebEvent("Uruchomienie systemu");
  for (uint8_t frame = 0; frame < 24; ++frame) {
    ui.showStartupAnimation(frame);
    delay(70);
  }
  sms_numbers_available = loadEmergencyNumbers(sms_numbers);
  if (!sms_numbers_available) {
    Serial.println("Nie mozna odczytac listy numerow SMS");
  }
  last_contacts_refresh = millis();
  const bool imu_started = crash.begin();
  imu_ok = imu_started;
  if (imu_ok) {
    Serial.println("MPU-6050: stabilizacja przed kalibracja...");
    const uint32_t stabilizationStart = millis();
    ui.showCalibrationMessage(0, false);
    while (millis() - stabilizationStart < 3000) {
      const uint8_t progress =
          (millis() - stabilizationStart) * 100 / 3000;
      ui.showCalibrationMessage(progress, false);
      delay(80);
    }
    imu_ok = crash.calibrate(showCalibrationProgress);
    logWebEvent(imu_ok ? "Kalibracja MPU-6050 zakonczona"
                       : "Blad kalibracji MPU-6050");
  }
  if (!imu_started) {
    logWebEvent("MPU-6050 niedostepny");
  }
  cabin.begin();
  gps.begin();
  gsm_ok = gsm.begin();
  gsm.refreshStatus();
  last_gsm_refresh = millis();
  logGsmStatusIfChanged();
  emergency.begin();
  demoWebBegin();
  logWebEvent("System gotowy do monitorowania");

  if (!imu_ok) {
    Serial.println("MPU-6050 niedostepny lub blad kalibracji");
  }
  if (!gsm_ok) {
    Serial.println("GSM niedostepny — tryb DEMO / log Serial");
  }
  digitalWrite(PIN_LED_OK, imu_ok ? HIGH : LOW);
}

void loop() {
  gps.poll();
  demoWebSetClock(gps.timestampText(), gps.timeSynced());
  const bool gps_fixed = gps.hasFix();
  if (gps_fixed != gps_was_fixed) {
    logWebEvent(gps_fixed ? "GPS: uzyskano FIX" : "GPS: utracono FIX");
    gps_was_fixed = gps_fixed;
  }
  demoWebLoop();
  emergency.tick();

  static uint32_t last_sos_btn = 0;
  static uint32_t last_cancel_btn = 0;

  if (buttonPressed(PIN_BTN_SOS, &last_sos_btn) || sos_web) {
    sos_web = false;
    emergency.onSos();
    logWebEvent("Reczne SOS - odliczanie uruchomione");
    menu_page = 0;
    Serial.println("SOS reczny");
  }
  const bool cancel_button_pressed =
      buttonPressed(PIN_BTN_CANCEL, &last_cancel_btn);
  if (cancel_button_pressed || cancel_web) {
    const SystemState state_before_cancel = emergency.state();
    if (cancel_button_pressed && !cancel_web &&
        emergency.state() == SystemState::Ready) {
      menu_page = (menu_page + 1) % 5;
    } else {
      menu_page = 0;
      emergency.onCancel();
    }
    if (state_before_cancel != SystemState::Ready &&
        emergency.state() == SystemState::Ready) {
      logWebEvent(state_before_cancel == SystemState::Countdown
                      ? "Odliczanie alarmu anulowane"
                      : "Alarm potwierdzony / zresetowany");
    }
    cancel_web = false;
    if (!cancel_button_pressed || emergency.state() != SystemState::Ready) {
      Serial.println("Anulowano / reset");
    }
  }

  if (emergency.state() != SystemState::Ready) {
    menu_page = 0;
  }

  const uint32_t now = millis();
  if (gsm_ok && now - last_gsm_refresh >= 10000) {
    last_gsm_refresh = now;
    gsm.refreshStatus();
    logGsmStatusIfChanged();
  }

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
      String event = "Detekcja: ";
      event += crashKindName(k);
      logWebEvent(event);
      emergency.onCrash(k);
      menu_page = 0;
    }
  }

  String extra_log;
  if (emergency.shouldSend()) {
    const uint8_t attempt = emergency.attemptNumber();
    if (attempt == 1) {
      sms_sent_total = 0;
      ui.show(SystemState::Sending, crash.accelG(), crash.rollDeg(),
              crash.pitchDeg(), crash.yawDeg(), gps.coordsText(), gps.satellites(),
              gsm.signalBars(), gsm.statusText(), gps.clockText(), sms_numbers,
              sms_numbers_available, 0, 0, getDisplayMenuData());
      demoWebPush(SystemState::Sending, emergency.kind(), crash.accelG(),
                  crash.rollDeg(), crash.pitchDeg(), crash.yawDeg(), gps.coordsText(),
                  gps.mapsUrl(), gps.diagnostics(), gsm.statusText(), gps.satellites(),
                  gsm.signalBars(), cabin.tempC(), cabin.tempValid(), 0,
                  "PROBA 1/2: wysylam SMS");
    } else {
      ui.show(SystemState::Retrying, crash.accelG(), crash.rollDeg(),
              crash.pitchDeg(), crash.yawDeg(), gps.coordsText(), gps.satellites(),
              gsm.signalBars(), gsm.statusText(), gps.clockText(), sms_numbers,
              sms_numbers_available, 0, 0, getDisplayMenuData());
      demoWebPush(SystemState::Retrying, emergency.kind(), crash.accelG(),
                  crash.rollDeg(), crash.pitchDeg(), crash.yawDeg(), gps.coordsText(),
                  gps.mapsUrl(), gps.diagnostics(), gsm.statusText(), gps.satellites(),
                  gsm.signalBars(), cabin.tempC(), cabin.tempValid(), 0,
                  "PROBA 2/2: ponawiam wysylke SMS");
      if (!gsm_ok) {
        gsm_ok = gsm.begin();
      }
      if (gsm_ok) {
        gsm.refreshStatus();
        last_gsm_refresh = millis();
        logGsmStatusIfChanged();
      }

    }

    const String sms = buildSms(emergency.kind());
    Serial.println("=== TRESC ALARMU ===");
    Serial.println(sms);
    uint8_t sent_count = 0;
    const bool ok =
        attempt == 1 ? gsm.sendToAll(sms, &sent_count)
                     : gsm.retryFailed(sms, &sent_count);
    sms_sent_total += sent_count;
    const bool partial = !ok && sms_sent_total > 0;
    const bool retry_available = !ok && gsm.hasPendingNumbers();
    emergency.markAttemptResult(ok, partial, retry_available);
    if (ok) {
      extra_log = "SMS wyslano do wszystkich numerow";
    } else if (attempt == 1 && retry_available) {
      extra_log = "PROBA 1 NIEUDANA: ";
      extra_log += gsm.lastError();
      extra_log += "; druga proba za 3 s";
    } else if (partial) {
      extra_log = "SMS wyslano do ";
      extra_log += String(sms_sent_total);
      extra_log += " numerow; pozostale nie";
    } else {
      extra_log = attempt == 2 ? "PROBA 2/2 NIEUDANA: "
                               : "SMS NIE WYSŁANO: ";
      extra_log += " ";
      extra_log += gsm.lastError();
    }
    demoWebPush(emergency.state(), emergency.kind(), crash.accelG(),
                crash.rollDeg(), crash.pitchDeg(), crash.yawDeg(), gps.coordsText(),
                gps.mapsUrl(), gps.diagnostics(), gsm.statusText(), gps.satellites(),
                gsm.signalBars(), cabin.tempC(), cabin.tempValid(),
                0, extra_log);
  }

  if (now - last_ui >= 250) {
    last_ui = now;
    SystemState st = emergency.state();
    if (!imu_ok && st == SystemState::Ready) {
      st = SystemState::Fault;
    }
    ui.show(st, crash.accelG(), crash.rollDeg(), crash.pitchDeg(),
            crash.yawDeg(), gps.coordsText(), gps.satellites(), gsm.signalBars(),
            gsm.statusText(), gps.clockText(), sms_numbers, sms_numbers_available,
            emergency.countdownSeconds(), menu_page, getDisplayMenuData());
    demoWebPush(st, emergency.kind(), crash.accelG(), crash.rollDeg(),
                crash.pitchDeg(), crash.yawDeg(), gps.coordsText(), gps.mapsUrl(),
                gps.diagnostics(), gsm.statusText(), gps.satellites(), gsm.signalBars(),
                cabin.tempC(), cabin.tempValid(),
                emergency.countdownSeconds(), "");
    digitalWrite(PIN_LED_OK, gps.hasFix() ? HIGH : ((now / 400) % 2));
  }
}
