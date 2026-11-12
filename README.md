# Car SOS System

Firmware for ESP32 DevKit using PlatformIO and Arduino.

## Setup

1. Copy `include/config.example.h` to `include/config.h`.
2. Set a private Wi-Fi access point password in `include/config.h` (at least 8 characters).
3. Build and upload the `esp32dev` environment with PlatformIO.
4. Connect to the `SOS-AUTO` Wi-Fi network and open `http://192.168.4.1`.
5. Add the real SMS recipient numbers in the web panel before testing alerts.

The local `include/config.h` is ignored by Git so device credentials and
recipient numbers are not committed. Configure the hardware power supplies
according to the specifications of the connected modules.
