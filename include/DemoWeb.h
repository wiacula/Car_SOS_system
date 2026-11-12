#pragma once

#include <Arduino.h>
#include "Types.h"

void demoWebBegin();
void demoWebLoop();
void demoWebPush(SystemState state,
                 CrashKind kind,
                 float g,
                 float roll,
                 float pitch,
                 const String& gps,
                 const String& gpsDiagnostics,
                 float temperatureC,
                 bool temperatureValid,
                 int countdown_s,
                 const String& logLine);
