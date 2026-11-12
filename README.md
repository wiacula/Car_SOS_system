# Car SOS System

Firmware for ESP32 DevKit using PlatformIO and Arduino.

## Setup

1. Copy `include/config.example.h` to `include/config.h`.
2. Set a private Wi-Fi access point password in `include/config.h` (at least 8 characters).
3. Build and upload the `esp32dev` environment with PlatformIO.
4. Connect to the `SOS-AUTO` Wi-Fi network and open `http://192.168.4.1`.
5. Add the real SMS recipient numbers in the web panel before testing alerts.

Keep the ESP32 and MPU-6050 still during the startup calibration screen:
the sensor settles for three seconds, then the accelerometer and gyroscope
offsets are sampled for about one second. The startup orientation becomes
the zero point for roll, pitch, and relative yaw. Yaw can drift over time
because the MPU-6050 has no magnetometer.

While monitoring, press the cancel button (GPIO 33) to cycle through the OLED
screens: main screen, saved SMS numbers, sensor readings, GPS status, and SIM/GSM
status. Press it on the SIM/GSM screen to return to the main screen. During an
active alarm, the same button keeps its cancel/acknowledge function.

The local `include/config.h` is ignored by Git so device credentials and
recipient numbers are not committed. Configure the hardware power supplies
according to the specifications of the connected modules.
