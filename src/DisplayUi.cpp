#include "DisplayUi.h"

bool DisplayUi::begin() {
  ready_ = oled_.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!ready_) {
    return false;
  }
  oled_.clearDisplay();
  oled_.setTextColor(SSD1306_WHITE);
  oled_.setTextSize(1);
  oled_.setCursor(0, 0);
  oled_.println("System ratunkowy");
  oled_.println("start...");
  oled_.display();
  return true;
}

void DisplayUi::show(SystemState state,
                    float g,
                    float roll,
                    float pitch,
                    const String& gpsLine,
                    const std::vector<String>& smsNumbers,
                    bool smsNumbersAvailable,
                    int countdown_s,
                    const String& extra) {
  if (!ready_) {
    return;
  }
  oled_.clearDisplay();
  oled_.setTextSize(1);
  oled_.setTextColor(SSD1306_WHITE);
  oled_.setCursor(4, 1);
  oled_.print("SOS AUTO");
  oled_.setCursor(91, 1);
  oled_.print("GPS");
  const bool gps_fix = gpsLine.length() > 0 && gpsLine.indexOf("BRAK FIX") < 0;
  oled_.drawCircle(119, 5, 3, SSD1306_WHITE);
  if (gps_fix) {
    oled_.fillCircle(119, 5, 2, SSD1306_WHITE);
  }
  oled_.drawFastHLine(2, 9, 124, SSD1306_WHITE);

  String stateLabel;
  bool alarm = false;
  switch (state) {
    case SystemState::Boot:
      stateLabel = "START";
      break;
    case SystemState::Ready:
      stateLabel = "GOTOWY";
      break;
    case SystemState::Countdown:
      stateLabel = "SOS ";
      stateLabel += countdown_s;
      stateLabel += "s";
      alarm = true;
      break;
    case SystemState::Sending:
      stateLabel = "WYSYLAM";
      alarm = true;
      break;
    case SystemState::Sent:
      stateLabel = "SMS OK";
      alarm = true;
      break;
    case SystemState::Fault:
      stateLabel = "AWARIA";
      break;
  }

  oled_.fillRoundRect(3, 11, 122, 17, 4, SSD1306_WHITE);
  oled_.fillCircle(13, 19, 6, SSD1306_BLACK);
  oled_.setTextColor(SSD1306_WHITE);
  oled_.setCursor(11, 15);
  oled_.print(alarm ? "!" : "+");
  oled_.setTextColor(SSD1306_BLACK);
  oled_.setTextSize(2);
  oled_.setCursor(24, 11);
  oled_.print(stateLabel);

  oled_.setTextSize(1);
  oled_.setTextColor(SSD1306_WHITE);
  oled_.drawRoundRect(2, 29, 124, 10, 2, SSD1306_WHITE);
  oled_.drawFastVLine(43, 30, 8, SSD1306_WHITE);
  oled_.drawFastVLine(84, 30, 8, SSD1306_WHITE);
  oled_.setCursor(5, 30);
  oled_.print("G ");
  oled_.print(g, 1);
  oled_.setCursor(47, 30);
  oled_.print("R ");
  oled_.print(roll, 0);
  oled_.setCursor(88, 30);
  oled_.print("P ");
  oled_.print(pitch, 0);

  if (extra.length() > 0) {
    oled_.fillCircle(6, 43, 2, SSD1306_WHITE);
    oled_.setCursor(12, 39);
    oled_.print(extra);
  }
  oled_.setCursor(3, 47);
  oled_.print(gpsLine);

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
