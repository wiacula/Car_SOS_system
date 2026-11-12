#pragma once

// --- Piny ESP32 DevKit ---
static const int PIN_SDA = 21;
static const int PIN_SCL = 22;

static const int PIN_GPS_RX = 16;
static const int PIN_GPS_TX = 17;

static const int PIN_GSM_RX = 26;
static const int PIN_GSM_TX = 27;
static const int PIN_GSM_RST = 4;

static const int PIN_BTN_SOS = 32;
static const int PIN_BTN_CANCEL = 33;
static const int PIN_BUZZER = 25;
static const int PIN_LED_ALARM = 2;
static const int PIN_LED_OK = 15;

static const int PIN_MQ2 = 34;
static const int PIN_DHT = 13;

static const int MQ2_ADC_THRESHOLD = 1800;
static const int MQ2_HIT_COUNT = 20;
static const uint32_t MQ2_WARMUP_MS = 90000;
static const float TEMP_ALARM_C = 40.0f;
static const int TEMP_HIT_COUNT = 3;
static const uint32_t DHT_PERIOD_MS = 2000;

static const uint32_t GPS_BAUD = 9600;
static const uint32_t GSM_BAUD = 9600;

static const float CRASH_G_THRESHOLD = 3.5f;
static const int CRASH_HIT_COUNT = 3;
static const float ROLLOVER_DEG = 60.0f;
static const uint32_t ROLLOVER_HOLD_MS = 400;
static const uint32_t STARTUP_IGNORE_MS = 3000;
static const uint32_t SAMPLE_MS = 10;

static const float COMP_FILTER_ALPHA = 0.98f;

static const uint32_t COUNTDOWN_MS = 15000;
static const uint32_t DEBOUNCE_MS = 40;

// Configure real recipient numbers in the web panel before testing SMS.
static const char* EMERGENCY_NUMBERS[] = {""};
static const int EMERGENCY_NUMBER_COUNT = 0;

// Empty string means the SIM card has no PIN.
static const char* SIM_PIN = "";

static const char* VEHICLE_ID = "Pojazd-inz-01";

static const bool WIFI_DEMO_ENABLED = true;
static const char* WIFI_AP_SSID = "SOS-AUTO";
// Replace this example password with a private password (minimum 8 characters).
static const char* WIFI_AP_PASS = "CHANGE_ME_123";

static const int OLED_WIDTH = 128;
static const int OLED_HEIGHT = 64;
static const int OLED_ADDR = 0x3C;
