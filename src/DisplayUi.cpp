#include "DisplayUi.h"

bool DisplayUi::begin() {
  ready_ = oled_.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!ready_) {
    return false;
  }
  return true;
}

void DisplayUi::showStartupAnimation(uint8_t frame) {
  if (!ready_) {
    return;
  }
  oled_.clearDisplay();
  oled_.drawRoundRect(1, 1, 126, 62, 7, SSD1306_WHITE);
  oled_.setTextColor(SSD1306_WHITE);
  oled_.setTextSize(1);
  oled_.setCursor(16, 7);
  oled_.print("SYSTEM RATUNKOWY");
  oled_.drawFastHLine(12, 17, 104, SSD1306_WHITE);

  oled_.drawLine(28, 40, 35, 40, SSD1306_WHITE);
  oled_.drawLine(35, 40, 42, 29, SSD1306_WHITE);
  oled_.drawLine(42, 29, 68, 29, SSD1306_WHITE);
  oled_.drawLine(68, 29, 77, 40, SSD1306_WHITE);
  oled_.drawLine(24, 40, 80, 40, SSD1306_WHITE);
  oled_.drawLine(28, 40, 28, 45, SSD1306_WHITE);
  oled_.drawLine(28, 45, 80, 45, SSD1306_WHITE);
  oled_.drawLine(80, 45, 80, 40, SSD1306_WHITE);
  oled_.fillCircle(40, 45, 4, SSD1306_WHITE);
  oled_.fillCircle(68, 45, 4, SSD1306_WHITE);

  oled_.drawCircle(101, 36, 8 + ((frame / 3) % 3), SSD1306_WHITE);
  oled_.setTextSize(1);
  oled_.setCursor(92, 33);
  oled_.print("SOS");

  oled_.drawRoundRect(13, 54, 102, 7, 3, SSD1306_WHITE);
  const int marker = 15 + (frame * 5) % 88;
  oled_.fillRoundRect(marker, 56, 12, 3, 1, SSD1306_WHITE);
  oled_.display();
}

void DisplayUi::showStartupSelfTest(const StartupSelfTestData& data) {
  if (!ready_) {
    return;
  }
  oled_.clearDisplay();
  oled_.setTextColor(SSD1306_WHITE);
  oled_.setTextSize(1);
  oled_.setCursor(13, 1);
  oled_.print("AUTOTEST STARTOWY");
  oled_.drawFastHLine(2, 9, 124, SSD1306_WHITE);

  oled_.setCursor(3, 12);
  oled_.print("MPU: ");
  oled_.print(data.imuReady ? "OK" : "BLAD");
  oled_.setCursor(68, 12);
  oled_.print("DHT: ");
  oled_.print(data.temperatureValid
                  ? "OK"
                  : (data.temperatureReadAttempted ? "BLAD" : "CZEKA"));

  oled_.setCursor(3, 21);
  oled_.print("MQ2: ");
  oled_.print(data.gasRaw);
  oled_.setCursor(72, 21);
  oled_.print(data.gasWarmedUp ? "GOTOWY" : "GRZANIE");

  oled_.setCursor(3, 30);
  oled_.print("GPS: ");
  oled_.print(data.gpsFix ? "FIX OK"
                          : (data.gpsDataReceived ? "SZUKA FIX"
                                                  : "BRAK DANYCH"));

  oled_.setCursor(3, 39);
  if (!data.gsmReady) {
    oled_.print("GSM: BLAD MODULU");
  } else if (!data.simReady) {
    oled_.print("GSM: BLAD SIM / PIN");
  } else if (!data.networkRegistered) {
    oled_.print("GSM: SZUKA SIECI");
  } else {
    oled_.print("GSM: OK ");
    oled_.print(data.gsmSignalBars);
    oled_.print("/4");
  }

  oled_.setCursor(3, 48);
  if (!data.smsNumbersAvailable) {
    oled_.print("NUMERY: BLAD PAMIECI");
  } else if (data.smsNumbersCount == 0) {
    oled_.print("NUMERY: BRAK");
  } else {
    oled_.print("NUMERY: OK (");
    oled_.print(data.smsNumbersCount);
    oled_.print(")");
  }

  oled_.drawFastHLine(2, 55, 124, SSD1306_WHITE);
  oled_.setCursor(3, 57);
  oled_.print("MONITORING AKTYWNY");
  oled_.display();
}

void DisplayUi::showCalibrationMessage(uint8_t progress, bool sampling) {
  if (!ready_) {
    return;
  }
  oled_.clearDisplay();
  oled_.drawRoundRect(1, 1, 126, 62, 6, SSD1306_WHITE);
  oled_.setTextColor(SSD1306_WHITE);
  oled_.setTextSize(2);
  oled_.setCursor(4, 7);
  oled_.print("KALIBRACJA");
  oled_.setCursor(10, 25);
  oled_.print("CZUJNIKOW");
  oled_.setTextSize(1);
  oled_.setCursor(7, 44);
  oled_.print(sampling ? "PROBKOWANIE" : "STABILIZACJA");
  oled_.setCursor(83, 44);
  oled_.print(progress);
  oled_.print("%");
  oled_.drawCircle(117, 43, 5, SSD1306_WHITE);
  static const int8_t spinnerX[] = {0, 1, 1, 1, 0, -1, -1, -1};
  static const int8_t spinnerY[] = {-1, -1, 0, 1, 1, 1, 0, -1};
  const uint8_t spinnerFrame = (millis() / 120) % 8;
  oled_.fillCircle(117 + spinnerX[spinnerFrame] * 4,
                   43 + spinnerY[spinnerFrame] * 4, 2, SSD1306_WHITE);
  oled_.drawRoundRect(8, 54, 112, 7, 3, SSD1306_WHITE);
  const int progressWidth = (108 * progress) / 100;
  if (progressWidth > 0) {
    oled_.fillRoundRect(10, 56, progressWidth, 3, 1, SSD1306_WHITE);
  }
  oled_.display();
}

void DisplayUi::show(SystemState state,
                    float g,
                    float roll,
                    float pitch,
                    float yaw,
                    const String& gpsLine,
                    uint8_t gpsSatellites,
                    uint8_t gsmSignalBars,
                    const String& gsmStatus,
                    const String& clockText,
                    const std::vector<String>& smsNumbers,
                    bool smsNumbersAvailable,
                    int countdown_s,
                    uint8_t menuPage,
                    const DisplayMenuData& menuData) {
  if (!ready_) {
    return;
  }
  if (menuPage > 0) {
    showMenu(menuPage, g, gpsLine, gpsSatellites, gsmSignalBars, gsmStatus,
             smsNumbers, smsNumbersAvailable, menuData);
    return;
  }

  oled_.clearDisplay();
  oled_.setTextSize(1);
  oled_.setTextColor(SSD1306_WHITE);
  oled_.setCursor(2, 1);
  oled_.print(clockText);
  oled_.setCursor(38, 1);
  oled_.print("GSM");
  for (uint8_t i = 0; i < 4; ++i) {
    const int height = 3 + i;
    const int x = 58 + i * 5;
    if (i < gsmSignalBars) {
      oled_.fillRect(x, 8 - height, 4, height, SSD1306_WHITE);
    } else {
      oled_.drawRect(x, 8 - height, 4, height, SSD1306_WHITE);
    }
  }
  oled_.setCursor(79, 1);
  oled_.print("SAT");
  oled_.setCursor(111, 1);
  if (gpsSatellites < 10) {
    oled_.print("0");
  }
  oled_.print(gpsSatellites);
  oled_.drawFastHLine(2, 9, 124, SSD1306_WHITE);

  String stateLabel;
  switch (state) {
    case SystemState::Boot:
      stateLabel = "START";
      break;
    case SystemState::Ready:
      stateLabel = "MONITORING";
      break;
    case SystemState::Countdown:
      stateLabel = "ALARM";
      stateLabel += " ";
      stateLabel += String(countdown_s);
      stateLabel += "s";
      break;
    case SystemState::Sending:
      stateLabel = "PROBA 1/2";
      break;
    case SystemState::RetryWait:
      stateLabel = "PROBA 1 NIEUDANA";
      break;
    case SystemState::Retrying:
      stateLabel = "PROBA 2/2";
      break;
    case SystemState::Sent:
      stateLabel = "WYSLANO SMS";
      break;
    case SystemState::SendPartial:
      stateLabel = "CZESC SMS OK";
      break;
    case SystemState::SendFailed:
      stateLabel = "BLAD WYSYLKI";
      break;
    case SystemState::Fault:
      stateLabel = "BLAD CZUJNIKA";
      break;
  }

  oled_.fillRoundRect(3, 11, 122, 17, 4, SSD1306_WHITE);
  oled_.setTextColor(SSD1306_BLACK);
  oled_.setTextSize(2);
  const size_t visible_chars = 9;
  const size_t gap_chars = 3;
  const size_t cycle_chars = stateLabel.length() + gap_chars;
  const size_t offset =
      stateLabel.length() > visible_chars ? (millis() / 300) % cycle_chars : 0;
  oled_.setCursor(5, 11);
  for (size_t i = 0; i < visible_chars; ++i) {
    const size_t position = (offset + i) % cycle_chars;
    oled_.write(position < stateLabel.length() ? stateLabel[position] : ' ');
  }

  oled_.setTextSize(1);
  oled_.setTextColor(SSD1306_WHITE);
  oled_.drawRoundRect(2, 29, 124, 10, 2, SSD1306_WHITE);
  oled_.drawFastVLine(33, 30, 8, SSD1306_WHITE);
  oled_.drawFastVLine(64, 30, 8, SSD1306_WHITE);
  oled_.drawFastVLine(95, 30, 8, SSD1306_WHITE);
  oled_.setCursor(4, 30);
  oled_.print("G");
  oled_.print(g, 1);
  oled_.setCursor(35, 30);
  oled_.print("R");
  oled_.print(roll, 0);
  oled_.setCursor(66, 30);
  oled_.print("P");
  oled_.print(pitch, 0);
  oled_.setCursor(97, 30);
  oled_.print("Y");
  oled_.print(yaw, 0);

  oled_.setCursor(3, 39);
  oled_.print("Dym: ");
  oled_.print(menuData.gasRaw);
  oled_.print(" Temp:");
  if (menuData.temperatureValid) {
    oled_.print(menuData.temperatureC, 1);
    oled_.print("C");
  } else {
    oled_.print("--");
  }

  const String& statusLine =
      (millis() / 3000) % 2 == 0 ? gsmStatus : gpsLine;
  oled_.setCursor(3, 47);
  oled_.print(statusLine);

  oled_.drawFastHLine(2, 56, 124, SSD1306_WHITE);
  oled_.setCursor(2, 56);
  if (!smsNumbersAvailable) {
    oled_.print("SMS: BLAD PAMIECI");
  } else if (smsNumbers.empty()) {
    oled_.print("SMS: BRAK NUMEROW");
  } else {
    const size_t index = (millis() / 6000) % smsNumbers.size();
    String ticker = "SMS ";
    ticker += String(index + 1);
    ticker += "/";
    ticker += String(smsNumbers.size());
    ticker += " ";
    ticker += smsNumbers[index];

    const size_t window_size = 20;
    if (ticker.length() <= window_size) {
      oled_.print(ticker);
    } else {
      const size_t scroll_length = ticker.length() + 4;
      const size_t offset = (millis() / 200) % scroll_length;
      for (size_t i = 0; i < window_size; ++i) {
        const size_t position = offset + i;
        oled_.write(position < ticker.length() ? ticker[position] : ' ');
      }
    }
  }
  oled_.display();
}

void DisplayUi::showMenu(uint8_t page,
                        float g,
                        const String& gpsLine,
                        uint8_t gpsSatellites,
                        uint8_t gsmSignalBars,
                        const String& gsmStatus,
                        const std::vector<String>& smsNumbers,
                        bool smsNumbersAvailable,
                        const DisplayMenuData& menuData) {
  oled_.clearDisplay();
  oled_.setTextColor(SSD1306_WHITE);
  oled_.setTextSize(1);
  oled_.setCursor(3, 1);
  oled_.print(page);
  oled_.print("/4 ");

  if (page == 1) {
    oled_.print("NUMERY SMS");
  } else if (page == 2) {
    oled_.print("CZUJNIKI");
  } else if (page == 3) {
    oled_.print("GPS");
  } else {
    oled_.print("SIM / GSM");
  }
  oled_.drawFastHLine(2, 10, 124, SSD1306_WHITE);

  if (page == 1) {
    if (!smsNumbersAvailable) {
      oled_.setCursor(4, 17);
      oled_.print("BLAD ODCZYTU NUMEROW");
    } else if (smsNumbers.empty()) {
      oled_.setCursor(4, 17);
      oled_.print("BRAK ZAPISANYCH NUMEROW");
    } else {
      const size_t rows = 5;
      const size_t pageCount = (smsNumbers.size() + rows - 1) / rows;
      const size_t listPage = (millis() / 3000) % pageCount;
      const size_t first = listPage * rows;
      const size_t last = min(first + rows, smsNumbers.size());
      for (size_t i = first; i < last; ++i) {
        oled_.setCursor(4, 12 + (i - first) * 9);
        oled_.print(i + 1);
        oled_.print(". ");
        oled_.print(smsNumbers[i]);
      }
    }
  } else if (page == 2) {
    if (menuData.imuReady) {
      oled_.setCursor(3, 12);
      oled_.print("AX ");
      oled_.print(menuData.accelXG, 2);
      oled_.print(" AY ");
      oled_.print(menuData.accelYG, 2);
      oled_.setCursor(3, 21);
      oled_.print("AZ ");
      oled_.print(menuData.accelZG, 2);
      oled_.print(" G ");
      oled_.print(g, 2);
      oled_.setCursor(3, 30);
      oled_.print("GX ");
      oled_.print(menuData.gyroXDps, 1);
      oled_.print(" GY ");
      oled_.print(menuData.gyroYDps, 1);
      oled_.setCursor(3, 39);
      oled_.print("GZ ");
      oled_.print(menuData.gyroZDps, 1);
      oled_.print(" d/s");
    } else {
      oled_.setCursor(3, 15);
      oled_.print("MPU: BRAK CZUJNIKA");
    }
    oled_.setCursor(3, 48);
    oled_.print("T:");
    if (menuData.temperatureValid) {
      oled_.print(menuData.temperatureC, 1);
    } else {
      oled_.print("--");
    }
    oled_.print("C DYM:");
    oled_.print(menuData.gasRaw);
  } else if (page == 3) {
    oled_.setCursor(3, 13);
    oled_.print("STATUS: ");
    oled_.print(menuData.gpsFix ? "FIX" : "BRAK FIX");
    oled_.setCursor(3, 22);
    oled_.print("SATELITY: ");
    oled_.print(gpsSatellites);
    oled_.setCursor(3, 31);
    oled_.print(gpsLine);
    oled_.setCursor(3, 40);
    const size_t visible = 20;
    const size_t length = menuData.gpsDiagnostics.length();
    const size_t cycle = length + 4;
    const size_t offset = length > visible ? (millis() / 220) % cycle : 0;
    for (size_t i = 0; i < visible; ++i) {
      const size_t position = offset + i;
      oled_.write(position < length ? menuData.gpsDiagnostics[position] : ' ');
    }
  } else {
    oled_.setCursor(3, 13);
    oled_.print("SIM: ");
    oled_.print(!menuData.gsmReady ? "BRAK MODULU"
                                   : (menuData.simReady ? "GOTOWA" : "BRAK / PIN"));
    oled_.setCursor(3, 22);
    oled_.print("SIEC: ");
    oled_.print(!menuData.gsmReady
                    ? "BRAK MODULU"
                    : (menuData.networkRegistered ? "ZAREJESTROWANA" : "BRAK"));
    oled_.setCursor(3, 31);
    oled_.print(gsmStatus);
    oled_.setCursor(3, 43);
    oled_.print("ZASIEG");
    for (uint8_t i = 0; i < 4; ++i) {
      const int height = 3 + i * 2;
      const int x = 56 + i * 7;
      if (i < gsmSignalBars) {
        oled_.fillRect(x, 53 - height, 4, height, SSD1306_WHITE);
      } else {
        oled_.drawRect(x, 53 - height, 4, height, SSD1306_WHITE);
      }
    }
  }

  oled_.drawFastHLine(2, 55, 124, SSD1306_WHITE);
  oled_.setCursor(3, 57);
  oled_.print("ANULUJ: DALEJ");
  oled_.display();
}
