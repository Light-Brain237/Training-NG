# Interrupt ESP32 (Unified Robot Control System)

## Overview
Unified ESP32-CAM system combining camera streaming, distance sensor integration, and motor control via web interface. Two-core architecture: Core 0 handles serial communication with Arduino (distance polling & motor commands), Core 1 manages camera/HTTP server.

## Code Structure

### main.ino (Arduino IDE Format)
**Global Variables:**
- `gpLed` (4) - LED on GPIO4
- `WiFiAddr` - Stores ESP32 IP address
- `currentDistance` - Latest distance reading
- `arduinoConnected` - Arduino connection status
- `lastArduinoResponse` - Timestamp of last Arduino response
- `distanceQueue` - FreeRTOS queue for distance data
- `serialMutex` - Mutex protecting Serial access

**Functions:**

1. **`serialTask(void *parameter)`**
   - Runs on Core 0
   - Polls distance every 500ms: sends `GET_DISTANCE` to Arduino
   - Waits 100ms for response in format `DISTANCE:<cm>`
   - Validates: 0 ≤ distance ≤ 400
   - Updates `currentDistance` and `arduinoConnected` status
   - Mutex-protected to prevent conflicts with motor commands

2. **`sendMotorCommand(char cmd)`**
   - Sends single character command to Arduino (F/B/L/R/Q/W/Z/X/A/D/S)
   - Waits 50ms for ACK response
   - Uses mutex for thread-safe Serial access

3. **`setup()`**
   - Disables brownout detector
   - Initializes Serial @ 9600 baud
   - Creates FreeRTOS mutex and queue
   - Configures LED (GPIO4)
   - Initializes camera (AI Thinker pinout):
     - PIXFORMAT_JPEG (native format)
     - Frame size: UXGA if PSRAM available, else SVGA
     - XCLK: 20MHz
   - Connects to WiFi
   - Calls `startCameraServer()` from app_httpd.cpp
   - Creates `serialTask` on Core 0 (priority 3)

4. **`loop()`**
   - Minimal: just 10ms delay
   - All work done in Core 0 task and HTTP handlers

---

### app_httpd.cpp (HTTP Server & UI)

**Data Structures:**
- `ra_filter_t` - Running average filter (unused in this version)
- `jpg_chunking_t` - JPEG streaming context

**Key HTTP Handlers:**

1. **`index_handler()` (GET /)**
   - Returns embedded HTML/CSS/JS UI
   - Single-page app with:
     - Live camera feed (image stream from port 81)
     - Distance display with color-coded proximity bar
     - Interactive joystick control
     - Manual buttons (Turn Left/Right, Light On/Off)
   - JS polls `/distance` every 500ms for updates
   - Sends commands via `/F /B /L /R /Q /W /Z /X /A /D /S`

2. **`distance_handler()` (GET /distance)**
   - Returns JSON: `{"distance":<cm>, "connected":<bool>}`
   - CORS enabled

3. **`stream_handler()` (GET /stream on port 81)**
   - MJPEG streaming endpoint
   - Continuous loop: captures frame → encodes JPEG → sends chunks
   - Multipart boundary used to separate frames

4. **Motor Command Handlers:**
   - `/F` or `/go` → Forward
   - `/B` or `/back` → Backward
   - `/L` or `/left` → Left
   - `/R` or `/right` → Right
   - `/Q` or `/goleft` → Diagonal top-left
   - `/W` or `/goright` → Diagonal top-right
   - `/Z` or `/backleft` → Diagonal bottom-left
   - `/X` or `/backright` → Diagonal bottom-right
   - `/A` or `/Turn_Left` → Rotate left
   - `/D` or `/Turn_Right` → Rotate right
   - `/S` or `/stop` → Stop

5. **LED Handlers:**
   - `/ledon` → digitalWrite(gpLed, HIGH)
   - `/ledoff` → digitalWrite(gpLed, LOW)

**`startCameraServer()` Function:**
- Starts HTTP server on port 80 (main UI)
- Registers 26 URI handlers (command endpoints + UI)
- Starts MJPEG stream server on port 81
- Both servers use default config with max 40 URI handlers

---

## Web UI Features
- **Live Camera**: MJPEG stream embedded in page
- **Distance Display**: Large numeric display + color-coded proximity bar
  - Red (<10cm): VERY CLOSE
  - Orange (10-30cm): Close
  - Yellow (30-100cm): Safe Distance
  - Green (>100cm): Clear
- **Joystick Control**: 8-direction + center-stop
  - Touch/mouse drag to control direction
  - Real-time color feedback
- **Manual Buttons**: Left/Right turn, Light toggle
- **Status Indicator**: Green (connected) or Red (disconnected)

---

## GPIO Configuration

### LED Control
- **GPIO4** (`gpLed`): LED indicator (HIGH=on, LOW=off)

### Camera Pins (AI Thinker ESP32-CAM)
```
Control Pins:
  PWDN:  32  (Power Down)
  RESET: -1  (Not used)
  XCLK:  0   (Master clock, 20MHz)
  
Data Pins:
  Y2: 5,   Y3: 18,  Y4: 19,  Y5: 21
  Y6: 36,  Y7: 39,  Y8: 34,  Y9: 35
  
Sync Pins:
  PCLK:  22  (Pixel clock)
  VSYNC: 25  (Vertical sync)
  HREF:  23  (Horizontal reference)
  
I2C:
  SDA: 26  (Camera control)
  SCL: 27  (Camera control)
```

---

## WiFi & Serial Configuration
```cpp
// WiFi Credentials (in main.ino)
const char* ssid = "TP-LINK_4E81";
const char* password = "Jk123456789";

// Serial Communication
Serial.begin(9600);  // Baud rate must match Arduino
```

---

## FreeRTOS Architecture
| Core | Responsibility | Priority |
|------|---|---|
| 0 | Serial polling task (serialTask) | 3 (High) |
| 1 | Camera capture + HTTP server | Default |

**Synchronization:**
- `serialMutex` protects all Serial.print/println calls
- `distanceQueue` stores distance values (optional, not used in current code)

---

## Compile & Upload (Arduino IDE)

1. **Open Arduino IDE** and select:
   - Board: `Tools > Board > ESP32 Arduino > AI Thinker ESP32-CAM`
   - Port: `Tools > Port > /dev/ttyUSB0`
   - Baud: `Tools > Upload Speed > 921600` (or 115200)

2. **Open Files** in Arduino IDE:
   - Open `main.ino` as the main sketch
   - Copy contents of `app_httpd.cpp` into a new tab (name it `app_httpd.ino`)
     - Remove `#include "Arduino.h"` from the top if present
   - This creates a sketch with two `.ino` tabs

3. **Compile**: `Sketch > Verify/Compile` (Ctrl+R)

4. **Upload**: `Sketch > Upload` (Ctrl+U)
   - Hold `IO0` button during upload if you see "Connecting" timeout

5. **Monitor Serial**: `Tools > Serial Monitor` (Ctrl+Shift+M)
   - Set baud rate to `9600` to see distance polling output

---

## Access
- **UI**: `http://<ESP32_IP>:80/`
- **Stream**: `http://<ESP32_IP>:81/stream`
- **API**: `http://<ESP32_IP>:80/distance`

---

## Requires
- ESP32-CAM (AI Thinker)
- Arduino with interrupt/arduino firmware
- Serial connection to Arduino (RX/TX)
