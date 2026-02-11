# Interrupt Arduino (Slave Motor & Sensor Controller)

## Overview
Arduino slave controller that receives commands from ESP32 master via Serial. Drives motors using PWM and shift register, measures distance with ultrasonic sensor, and implements autonomous obstacle avoidance when moving forward.

## Code Structure

### Global Variables
- **PWM Control Pins:**
  - `PWM1_PIN` (5) - Motor speed PWM A
  - `PWM2_PIN` (6) - Motor speed PWM B

- **74HCT595N Shift Register Pins:**
  - `SHCP_PIN` (2) - Shift register clock (shift)
  - `STCP_PIN` (4) - Storage register clock (latch)
  - `DATA_PIN` (8) - Serial data input
  - `EN_PIN` (7) - Enable pin

- **Ultrasonic Sensor (HC-SR04):**
  - `Trig` (12) - Trigger pin (output)
  - `Echo` (13) - Echo pin (input)

- **Movement Control Values** (shift register patterns):
  - `Forward` (92) - Forward direction
  - `Backward` (163) - Backward direction
  - `Left` (149) - Left movement
  - `Right` (106) - Right movement
  - `Top_Left` (20) - Diagonal top-left
  - `Top_Right` (72) - Diagonal top-right
  - `Bottom_Left` (129) - Diagonal bottom-left
  - `Bottom_Right` (34) - Diagonal bottom-right
  - `Turn_Left` (172) - Rotate left
  - `Turn_Right` (83) - Rotate right
  - `Stop` (0) - Stop all motors

- **Runtime Variables:**
  - `distance` - Latest distance reading (cm)
  - `currentCmd` - Currently executing command
  - `lastDistanceRead` - Timestamp of last distance measurement
  - `DISTANCE_READ_INTERVAL` (100ms) - Distance polling interval

### Functions

1. **`Motor(int Dir, int Speed1)`**
   - Sets motor direction and speed
   - `Dir`: 8-bit pattern from shift register (direction control)
   - `Speed1`: 0-255 PWM value (motor speed)
   - Drives PWM1_PIN and PWM2_PIN with same speed value
   - Uses shiftOut() to send direction bits to 74HCT595N

2. **`int SR04(int Trig, int Echo)`**
   - Measures distance using HC-SR04 ultrasonic sensor
   - Sends 15µs pulse on Trig pin
   - Measures Echo pulse width
   - Converts to centimeters: `cm = pulseIn(Echo) / 58.8`
   - Returns distance (0-400cm range)
   - 10ms delay between measurements

3. **`void AvoidingObstacles()`**
   - Autonomous obstacle avoidance logic
   - Triggered when command is 'F' (forward)
   - If `distance > 20cm`: Continue forward at speed 250
   - If `distance ≤ 20cm`: Move backward 500ms → turn left 500ms → stop
   - Sets `currentCmd = 'S'` after avoidance complete

4. **`void ExecuteCommand(char cmd)`**
   - Executes motor command based on single character
   - Commands:
     - `F` - Forward with obstacle avoidance (calls AvoidingObstacles)
     - `B` - Backward
     - `L` - Move left
     - `R` - Move right
     - `A` - Turn left
     - `D` - Turn right
     - `Q` - Top-left diagonal
     - `W` - Top-right diagonal
     - `Z` - Bottom-left diagonal
     - `X` - Bottom-right diagonal
     - `S` - Stop
   - All movements use speed 250 (97.6% PWM)
   - Stores command in `currentCmd` global variable

5. **`void ProcessSerialCommands()`**
   - Reads and processes incoming Serial data
   - Waits for newline-terminated strings
   - Two message types:
     1. **Distance Request:** `GET_DISTANCE`
        - Response: `DISTANCE:<cm>` (e.g., `DISTANCE:45`)
     2. **Motor Command:** Single character (F/B/L/R/A/D/Q/W/Z/X/S)
        - Calls `ExecuteCommand()`
        - Response: `ACK:<cmd>` (e.g., `ACK:F`)
   - Ignores unknown commands

6. **`void setup()`**
   - Initializes Serial @ 9600 baud
   - Configures all pins as OUTPUT/INPUT
   - Motors disabled (Stop command, speed 0)
   - Sends `ARDUINO_READY` on Serial for handshake
   - 100ms delay for Serial stabilization

7. **`void loop()`**
   - Main execution loop (10ms per iteration)
   - Distance measurement: Every 100ms
     - Calls `SR04(Trig, Echo)` to get distance
     - If `currentCmd == 'F'`: Call `AvoidingObstacles()` for real-time obstacle detection
   - Serial processing: Every iteration
     - Calls `ProcessSerialCommands()` to handle master requests

---

## Master-Slave Protocol

### Communication Flow
**ESP32 (Master) ←→ Arduino (Slave) via Serial @ 9600 baud**

**Request: Distance Polling**
```
Master → Slave:  GET_DISTANCE\n
Slave → Master:  DISTANCE:45\n
```

**Request: Motor Command**
```
Master → Slave:  F\n
Slave → Master:  ACK:F\n
```

### Response Times
- Distance measurement: ~10ms (sensor + processing)
- Command acknowledgment: Immediate
- Polling interval: 100ms (distance), continuous (commands)

---

## Hardware Configuration

### Motor Control
- **PWM Pins:** 5 (A), 6 (B) - 0-255 speed control
- **Direction Pins:** 74HCT595N shift register
- **Speed Used:** 250/255 (~98% PWM for all movements)

### Obstacle Avoidance
- **Sensor:** HC-SR04 ultrasonic (Trig=12, Echo=13)
- **Range:** 2-400cm
- **Threshold:** 20cm (stop at <20cm, move at >20cm)
- **Reaction:** Backward 500ms → Turn left 500ms → Stop

### Shift Register (74HCT595N)
- **Data Pin:** 8 (MSBFIRST format)
- **Shift Clock (SHCP):** 2
- **Latch Clock (STCP):** 4
- **Enable:** 7 (held LOW during operation)
- **Output:** 8-bit direction patterns on Q0-Q7

---

## Compile & Upload (Arduino IDE)

1. **Open Arduino IDE** and select:
   - Board: `Tools > Board > Arduino AVR Boards > Arduino Uno`
   - Port: `Tools > Port > /dev/ttyUSB1`
   - Baud: `Tools > Upload Speed > 115200`

2. **Open File:** `main.ino` from `/src/interrupt/arduino/`

3. **Compile:** `Sketch > Verify/Compile` (Ctrl+R)

4. **Upload:** `Sketch > Upload` (Ctrl+U)

5. **Monitor Serial:** `Tools > Serial Monitor` (Ctrl+Shift+M)
   - Set baud rate to `9600`
   - Should see: `ARDUINO_READY`
   - Distance readings and ACK messages appear as master sends commands

---

## Requires
- Arduino Uno (ATmega328P)
- 74HCT595N 8-bit shift register (for direction control)
- HC-SR04 ultrasonic distance sensor
- Motors with motor driver (PWM controlled)
- Serial connection to ESP32 (RX/TX lines)

---

## Notes
- **Default Speed:** 250/255 (98% PWM) - can be adjusted in ExecuteCommand()
- **Obstacle Avoidance:** Only active when `currentCmd == 'F'` (forward)
- **Serial Format:** Newline-terminated strings (for distance), single char (for commands)
- **Distance Polling:** Every 100ms in main loop, always available via `GET_DISTANCE` request
- **DISTANCE_READ_INTERVAL:** Controls autonomous distance measurement frequency (currently 100ms)