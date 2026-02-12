# P5-Dashboard — ESP32-CAM (REST API Firmware)

## Overview

Adapted from P4-Interrupt. The ESP32-CAM exposes **CORS-enabled REST endpoints** for telemetry and motor commands instead of serving an embedded HTML interface. The web UI is handled by the React frontend.

Two-core FreeRTOS architecture:

- **Core 0**: Serial task — polls Arduino for distance every 500 ms
- **Core 1**: HTTP server (port 80) + MJPEG stream (port 81)

## Code Structure

### main.ino

**Global Variables:**

- `gpLed` (4) — LED on GPIO4
- `WiFiAddr` — ESP32 IP address string
- `currentDistance` — latest distance reading (cm)
- `arduinoConnected` — Arduino connection status
- `lastArduinoResponse` — timestamp of last valid response
- `lastMotorCmd` — last motor command sent (`'S'` = Stop)
- `distanceQueue` — FreeRTOS queue for distance data
- `serialMutex` — mutex protecting Serial access

**Functions:**

1. **`serialTask(void *parameter)`**
    - Runs on Core 0, priority 3
    - Sends `GET_DISTANCE` every 500 ms via mutex-protected Serial
    - Parses `DISTANCE:<cm>` response (100 ms timeout)
    - Validates range 0–400 cm
    - Updates `currentDistance`, `arduinoConnected`, `lastArduinoResponse`

2. **`sendMotorCommand(char cmd)`**
    - Sends single-char command (F/B/L/R/S) to Arduino
    - Waits 50 ms for `ACK:<cmd>` response
    - Updates `lastMotorCmd`
    - Thread-safe via `serialMutex`

3. **`setup()`**
    - Disables brownout detector
    - Serial @ 9600 baud
    - Creates FreeRTOS mutex + queue
    - Camera init (AI Thinker pinout, JPEG, CIF frame size)
    - WiFi connect → prints API URL
    - Starts HTTP server via `startCameraServer()`
    - Creates `serialTask` pinned to Core 0

4. **`loop()`**
    - 10 ms delay; all work in Core 0 task and HTTP handlers

---

### app_httpd.cpp

**API Endpoints (port 80):**

| Method  | Path           | Response                                           |
| ------- | -------------- | -------------------------------------------------- |
| GET     | /api/telemetry | `{ distance, motorState, connected, uptimeMs }`    |
| GET     | /api/status    | `{ ssid, ip, freeHeap, uptimeMs }`                 |
| GET     | /command?cmd=X | P4-compatible motor command → `{ "status": "ok" }` |
| POST    | /api/command   | `{ "cmd": "F" }` → `{ "status": "ok" }`            |
| OPTIONS | /api/command   | CORS preflight (204 No Content)                    |

**MJPEG Stream (port 81):**

| Method | Path    | Description             |
| ------ | ------- | ----------------------- |
| GET    | /stream | Continuous MJPEG stream |

**CORS Headers** (all endpoints):

```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, OPTIONS
Access-Control-Allow-Headers: Content-Type
```

**Key Differences from P4:**

- No embedded HTML — no `index_handler()`
- Added `/api/telemetry` (JSON telemetry endpoint)
- Added `/api/status` (system info endpoint)
- Added `POST /api/command` with JSON body parsing
- Added OPTIONS preflight handler for CORS
- All responses include CORS headers
- `lastMotorCmd` tracked for telemetry reporting

---

## GPIO Configuration

### Camera Pins (AI Thinker ESP32-CAM)

```
PWDN: 32    RESET: -1   XCLK: 0
Y2: 5   Y3: 18  Y4: 19  Y5: 21
Y6: 36  Y7: 39  Y8: 34  Y9: 35
PCLK: 22   VSYNC: 25   HREF: 23
SDA: 26    SCL: 27
```

### LED

- **GPIO4** (`gpLed`): onboard LED

---

## WiFi Configuration

Update these in `main.ino` before flashing:

```cpp
const char *ssid     = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";
```

---

## Compile & Upload (Arduino IDE)

1. Board: `AI Thinker ESP32-CAM`
2. Port: `/dev/ttyUSB0`
3. Open `main.ino`, add `app_httpd.cpp` as a second tab
4. Compile (Ctrl+R) → Upload (Ctrl+U)
5. Serial Monitor @ 9600 baud → note the IP address

---

## Requires

- ESP32-CAM (AI Thinker) with OV2640
- Arduino with P4-Interrupt slave firmware
- Serial connection: ESP32 TX (GPIO1) → Arduino RX, ESP32 RX (GPIO3) → Arduino TX
- Common GND
