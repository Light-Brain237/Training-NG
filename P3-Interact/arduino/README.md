# Arduino Motor Controller - Interactive Web Control

## Overview
Arduino motor controller that receives Serial commands and drives DC motors with PWM speed control and a 74HCT595N shift register for direction control.

## Hardware Requirements

### Components
- Arduino UNO (ATmega328P)
- 74HCT595N shift register
- DC motor driver circuit
- DC motors
- USB cable for programming and Serial communication

### Pin Configuration
```
PWM Control Pins:
  - Pin 5  : PWM1_PIN (Motor PWM channel 1)
  - Pin 6  : PWM2_PIN (Motor PWM channel 2)

Shift Register Pins (74HCT595N):
  - Pin 2  : SHCP_PIN (Shift register clock)
  - Pin 7  : EN_PIN (Motor driver enable, active LOW)
  - Pin 8  : DATA_PIN (Serial data input)
  - Pin 4  : STCP_PIN (Storage register clock/latch)
```

## Serial Communication

### Settings
- **Baud Rate**: 9600
- **Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)

### Command Protocol

The Arduino listens for two types of commands:

#### 1. Speed Commands
**Format**: `SPEED:XXX\n`
- `XXX` = PWM value (0-255)
- Example: `SPEED:191\n` sets speed to 191 (~75%)

#### 2. Direction Commands
**Format**: `DIR:X\n`
- `F` = Forward
- `B` = Backward
- `S` = Stop

**Examples:**
```
SPEED:128\n     → Set speed to 50%
DIR:F\n         → Move forward at current speed (or default 150)
DIR:B\n         → Move backward at current speed (or default 150)
DIR:S\n         → Stop motors
```

## Code Structure

### Global State
```cpp
int currentSpeed = 0;               // Current PWM (0-255)
int currentDirection = Stop;        // Current direction
String directionName = "STOPPED";   // Human-readable state
```

### Main Functions

**`setMotor(int direction, int speed)`**
- Enables motor driver (EN_PIN LOW)
- Sets PWM on both channels
- Shifts direction byte to 74HCT595N
- Updates `directionName` and prints debug status

**`handleSpeedCommand(String value)`**
- Parses PWM value
- Clamps to 0-255 range
- Applies speed using current direction

**`handleDirectionCommand(String value)`**
- Parses `F/B/S`
- Uses current speed or default 150 when speed is 0
- Updates motor direction and speed

**`processSerialCommand()`**
- Reads a newline-terminated command
- Routes to SPEED or DIR handlers
- Logs unknown commands

### Direction Values (Shift Register)
- **Forward** = 92 (binary: 01011100)
- **Backward** = 163 (binary: 10100011)
- **Stop** = 0 (binary: 00000000)

## Debug Output

Examples from Serial:
```
[CMD] Speed command received: 191
[MOTOR] Direction: FORWARD | Speed: 191 (74%)

[CMD] Direction command received: B
[MOTOR] Direction: BACKWARD | Speed: 191 (74%)

[CMD] Direction command received: S
[MOTOR] Direction: STOPPED | Speed: 0 (0%)
```

## Arduino IDE Upload

1. **Open Arduino IDE** and select:
  - Board: `Tools > Board > Arduino AVR Boards > Arduino Uno`
  - Port: `Tools > Port > /dev/ttyUSB1`

2. **Open File:** `main.cpp` from `/src/interact/arduino/`
  - Arduino IDE will open it as a sketch

3. **Compile:** `Sketch > Verify/Compile` (Ctrl+R)

4. **Upload:** `Sketch > Upload` (Ctrl+U)

5. **Serial Monitor:** `Tools > Serial Monitor` (Ctrl+Shift+M)
  - Set baud rate to `9600`

## Notes
- Default direction speed is **150** if speed is 0 and a direction command is received.
- Speed and direction are persistent between commands.
- Commands must end with `\n` (newline) to be parsed.
