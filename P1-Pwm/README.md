# PWM Speed Control Program

## Overview
This program demonstrates PWM (Pulse Width Modulation) speed control for DC motors using an Arduino UNO with a 74HCT595N shift register motor driver. It allows real-time speed adjustment through serial commands.

## What is PWM?
PWM (Pulse Width Modulation) controls motor speed by rapidly switching power ON and OFF:
- **Duty Cycle**: The percentage of time the signal is HIGH
  - 0% = Motor OFF
  - 50% = Half speed (signal HIGH 50% of the time)
  - 100% = Full speed (signal always HIGH)

For Arduino, `analogWrite()` uses values 0-255:
- `0` = 0% duty cycle (OFF)
- `128` = 50% duty cycle (half speed)
- `255` = 100% duty cycle (full speed)

## Hardware Setup

### Components
# PWM Speed Control (Arduino UNO)

## Overview
Controls DC motor speed and direction using PWM on pins 5/6 and a 74HCT595N shift register for direction control. Commands are sent over Serial.

## Hardware
- Arduino UNO
- 74HCT595N shift register
- Motor driver + DC motors

### Pin Map
```
PWM:
  PWM1_PIN = 5
  PWM2_PIN = 6

Shift Register (74HCT595N):
  SHCP_PIN = 2
  EN_PIN   = 7
  DATA_PIN = 8
  STCP_PIN = 4
```

## Commands
Speed:
- `L` = Low
- `M` = Medium (default)
- `H` = High

Movement:
- `F` = Forward
- `B` = Backward
- `S` = Stop

## Speed Levels (from code)
| Command | PWM Value | Approx. Duty Cycle |
|---------|-----------|--------------------|
| L       | 98        | ~38%               |
| M       | 166       | ~65%               |
| H       | 255       | 100%               |

> Note: If you want LOW to be 15%, change `SPEED_LOW` in pwm.ino to `38`.

## Behavior
- Speed changes apply immediately if motors are running.
- If stopped, the speed change is stored for the next move.

## Serial Monitor
- **Baud**: 9600
- **Line Ending**: Any

## Build & Upload (PlatformIO)
```
platformio run -e pwm_test --target upload --upload-port /dev/ttyUSB1
```

## Serial Monitor (PlatformIO)
```
platformio device monitor --port /dev/ttyUSB1 --baud 9600
```

## Files
- pwm.cpp: PlatformIO source
- pwm.ino: Arduino IDE source

---
**Board**: Arduino UNO (ATmega328P)
```
