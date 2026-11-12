#include "GpsModule.h"

bool GpsModule::begin() {
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
