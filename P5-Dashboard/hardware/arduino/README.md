# P5-Dashboard — Arduino (Slave Motor & Sensor Controller)

## Overview

The Arduino firmware for P5-Dashboard is **identical to P4-Interrupt**. The same slave controller receives commands from the ESP32 master via Serial, drives motors, measures distance, and performs obstacle avoidance.

> **No changes are needed.** If you have already flashed the P4-Interrupt Arduino firmware, it works as-is with the P5-Dashboard ESP32 firmware.

## Why No Changes?

The P5-Dashboard module only modifies the **ESP32 side** (replacing the embedded web interface with a REST API). The Serial protocol between ESP32 and Arduino remains the same:

| Direction       | Message           | Example          |
| --------------- | ----------------- | ---------------- |
| ESP32 → Arduino | `GET_DISTANCE\n`  | request distance |
| Arduino → ESP32 | `DISTANCE:<cm>\n` | `DISTANCE:45\n`  |
| ESP32 → Arduino | `<cmd>\n`         | `F\n`            |
| Arduino → ESP32 | `ACK:<cmd>\n`     | `ACK:F\n`        |

## Firmware Source

The `main.ino` in this directory is a copy of P4-Interrupt's Arduino slave firmware (unchanged).

## Compile & Upload

1. Open `main.ino` in Arduino IDE
2. Board: **Arduino Uno**
3. Port: `/dev/ttyUSB1` (or your Arduino's port)
4. Upload (Ctrl+U)
5. Serial Monitor at **9600 baud** → should see `ARDUINO_READY`

See [P4-Interrupt Arduino README](../../P4-Interrupt/arduino/README.md) for full details on pin configuration, motor control, and obstacle avoidance.
