#include "GpsModule.h"

#include <time.h>

static int64_t daysSinceUnixEpoch(int year, unsigned int month,
                                  unsigned int day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned int yearOfEra = static_cast<unsigned int>(year - era * 400);
  const int adjustedMonth =
      static_cast<int>(month) + (month > 2 ? -3 : 9);
  const unsigned int dayOfYear =
      static_cast<unsigned int>((153 * adjustedMonth + 2) / 5) + day - 1;
  const unsigned int dayOfEra =
      yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
  return static_cast<int64_t>(era) * 146097 + dayOfEra - 719468;
}

bool GpsModule::begin() {
  timezone_ready_ = setenv("TZ", LOCAL_TIMEZONE, 1) == 0;
  if (timezone_ready_) {
    tzset();
  } else {
    Serial.println("Blad ustawienia strefy czasowej; uzywam UTC");
  }
  Serial2.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  return true;
}

void GpsModule::poll() {
  while (Serial2.available() > 0) {
    const char data = static_cast<char>(Serial2.read());
    last_rx_ms_ = millis();
    gps_.encode(data);
  }
}

bool GpsModule::hasFix() const {
  return gps_.location.isValid() && gps_.location.age() < 5000;
}

bool GpsModule::timeSynced() {
  return gps_.date.isValid() && gps_.time.isValid() &&
         gps_.date.age() < 5000 && gps_.time.age() < 5000;
}

String GpsModule::timestampText() {
  if (timeSynced()) {
    const int year = gps_.date.year();
    const unsigned int month = gps_.date.month();
    const unsigned int day = gps_.date.day();
    const unsigned int hour = gps_.time.hour();
    const unsigned int minute = gps_.time.minute();
    const unsigned int second = gps_.time.second();

    if (timezone_ready_) {
      const int64_t epoch_seconds =
          daysSinceUnixEpoch(year, month, day) * 86400 +
          hour * 3600 + minute * 60 + second;
      const time_t epoch = static_cast<time_t>(epoch_seconds);
      struct tm local_time = {};
      if (localtime_r(&epoch, &local_time) != nullptr) {
        String timestamp = String(local_time.tm_year + 1900);
        timestamp += "-";
        if (local_time.tm_mon + 1 < 10) {
          timestamp += "0";
        }
        timestamp += String(local_time.tm_mon + 1);
        timestamp += "-";
        if (local_time.tm_mday < 10) {
          timestamp += "0";
        }
        timestamp += String(local_time.tm_mday);
        timestamp += " ";
        if (local_time.tm_hour < 10) {
          timestamp += "0";
        }
        timestamp += String(local_time.tm_hour);
        timestamp += ":";
        if (local_time.tm_min < 10) {
          timestamp += "0";
        }
        timestamp += String(local_time.tm_min);
        timestamp += ":";
        if (local_time.tm_sec < 10) {
          timestamp += "0";
        }
        timestamp += String(local_time.tm_sec);
        timestamp += local_time.tm_isdst > 0 ? " CEST" : " CET";
        return timestamp;
      }
    }

    String timestamp = String(year);
    timestamp += "-";
    if (month < 10) {
      timestamp += "0";
    }
    timestamp += String(month);
    timestamp += "-";
    if (day < 10) {
      timestamp += "0";
    }
    timestamp += String(day);
    timestamp += " ";
    if (hour < 10) {
      timestamp += "0";
    }
    timestamp += String(hour);
    timestamp += ":";
    if (minute < 10) {
      timestamp += "0";
    }
    timestamp += String(minute);
    timestamp += ":";
    if (second < 10) {
      timestamp += "0";
    }
    timestamp += String(second);
    timestamp += " UTC";
    return timestamp;
  }

  const uint32_t elapsedSeconds = millis() / 1000;
  const uint32_t hours = elapsedSeconds / 3600;
  const uint8_t minutes = (elapsedSeconds / 60) % 60;
  const uint8_t seconds = elapsedSeconds % 60;
  String timestamp = "T+";
  if (hours < 10) {
    timestamp += "0";
  }
  timestamp += String(hours);
  timestamp += ":";
  if (minutes < 10) {
    timestamp += "0";
  }
  timestamp += String(minutes);
  timestamp += ":";
  if (seconds < 10) {
    timestamp += "0";
  }
  timestamp += String(seconds);
  return timestamp;
}

String GpsModule::clockText() {
  const String timestamp = timestampText();
  const int separator = timestamp.indexOf(' ');
  if (separator >= 0 && separator + 6 <= timestamp.length()) {
    return timestamp.substring(separator + 1, separator + 6);
  }
  return "--:--";
}

String GpsModule::diagnostics() {
  const uint32_t chars = gps_.charsProcessed();
  if (chars == 0) {
    return "Brak danych UART; sprawdz zasilanie, TX/RX i baud 9600";
  }

  const uint32_t silent_ms = millis() - last_rx_ms_;
  if (silent_ms > 3000) {
    String status = "Brak nowych danych od ";
    status += String(silent_ms / 1000);
    status += " s; sprawdz polaczenia GPS";
    return status;
  }

  String status = "UART odbiera ";
  status += String(chars);
  status += " znakow; NMEA OK ";
  status += String(gps_.passedChecksum());
  status += ", bledne sumy ";
  status += String(gps_.failedChecksum());

  if (hasFix()) {
    status += "; FIX; satelity ";
    status += String(gps_.satellites.value());
  } else if (gps_.passedChecksum() > 0) {
    status += "; poprawne NMEA, oczekiwanie na FIX; satelity ";
    status += String(gps_.satellites.value());
  } else {
    status += "; brak poprawnych ramek - sprawdz baud 9600 i TX/RX";
  }
  return status;
}

double GpsModule::lat() { return gps_.location.lat(); }
double GpsModule::lon() { return gps_.location.lng(); }
uint8_t GpsModule::satellites() { return gps_.satellites.value(); }

String GpsModule::mapsUrl() {
  if (!hasFix()) {
    return "";
  }
  String url = "https://maps.google.com/?q=";
  url += String(lat(), 6);
  url += ",";
  url += String(lon(), 6);
  return url;
}

String GpsModule::coordsText() {
  if (!hasFix()) {
    return "GPS: BRAK FIX";
  }
  String s;
  s += String(lat(), 5);
  s += ", ";
  s += String(lon(), 5);
  return s;
}
