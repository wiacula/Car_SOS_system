#pragma once

#include <Arduino.h>
#include "Types.h"

void demoWebBegin();
void demoWebLoop();
void demoWebSetClock(const String& timestamp, bool gpsTimeSynced);
void demoWebLog(const String& message);
void demoWebPush(SystemState state,
                 CrashKind kind,
                 float g,
                 float roll,
                 float pitch,
                 float yaw,
                 const String& gps,
                 const String& gpsDiagnostics,
                 const String& gsmDiagnostics,
                 uint8_t gpsSatellites,
                 uint8_t gsmSignalBars,
                 float temperatureC,
                 bool temperatureValid,
                 int countdown_s,
                 const String& logLine);
