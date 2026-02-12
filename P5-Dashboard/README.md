# P5-Dashboard — IoT Robot Dashboard (Express + React)

**Module 5 of the ESP32-Arduino IoT Robotics Training**

This module replaces the ESP32-hosted web interface from earlier modules with a professional full-stack dashboard built with **Node.js Express** (backend) and **React + TypeScript** (frontend). The ESP32 exposes a CORS-enabled REST API instead of serving HTML, and an Express server polls telemetry and broadcasts it to connected dashboards via **WebSocket**.

---

## Architecture

```
┌──────────────────┐        Serial (9600 baud)        ┌─────────────┐
│   ESP32-CAM      │ ◄──────────────────────────────► │ Arduino UNO │
│   (AI Thinker)   │   GET_DISTANCE / ACK:F / etc.    │  (Slave)    │
│                  │                                    └─────────────┘
│  Port 80: REST   │◄── HTTP GET/POST ──┐
│  Port 81: MJPEG  │                    │
└──────────────────┘                    │
                                        ▼
                              ┌───────────────────┐
                              │  Express Backend   │
                              │  (Node.js :3001)   │
                              │                    │
                              │  Polls /api/telemetry every 500ms
                              │  Forwards POST /api/command
                              │  Broadcasts via WebSocket
                              └────────┬──────────┘
                                       │ WebSocket
                                       ▼
                              ┌───────────────────┐
                              │  React Dashboard   │
                              │  (Vite :5173)      │
                              │                    │
                              │  Camera feed       │
                              │  Live distance chart│
                              │  Motor controls    │
                              │  Status alerts     │
                              └───────────────────┘
```

---

## Folder Structure

```
P5-Dashboard/
├── hardware/
│   ├── arduino/
│   │   ├── main.ino          # Slave controller (unchanged from P4)
│   │   └── README.md
│   └── esp32/
│       ├── main.ino          # FreeRTOS setup, serial task, WiFi, camera init
│       ├── app_httpd.cpp     # CORS REST API: /api/telemetry, /api/command, /stream
│       └── README.md
├── backend/
│   ├── package.json
│   ├── server.js         # Express + WebSocket server
│   ├── .env.example
│   └── README.md
├── frontend/
│   ├── package.json
│   ├── vite.config.ts
│   ├── tsconfig.json
│   ├── index.html
│   ├── .env.example
│   ├── README.md
│   └── src/
│       ├── main.tsx
│       ├── App.tsx           # Main layout (grid)
│       ├── App.css           # Dark theme styles
│       ├── vite-env.d.ts
│       ├── hooks/
│       │   └── useWebSocket.ts   # WebSocket hook with auto-reconnect
│       └── components/
│           ├── CameraFeed.tsx     # MJPEG stream embed
│           ├── TelemetryChart.tsx # Recharts live distance graph
│           ├── ControlPanel.tsx   # Directional buttons (F/B/L/R/S)
│           └── StatusBar.tsx      # Connection status + distance alerts
└── README.md
```

---

## Prerequisites

### Software

| Tool        | Version | Purpose                        |
| ----------- | ------- | ------------------------------ |
| Node.js     | ≥ 18    | Express backend                |
| npm         | ≥ 9     | Package manager                |
| VS Code     | Latest  | Code editor                    |
| Arduino IDE | ≥ 2.0   | Arduino & ESP32 firmware flash |
| Chrome      | Latest  | Dashboard testing              |

### Node.js Installation

1. **Download** from [https://nodejs.org](https://nodejs.org) — choose the **LTS** version (v18 or newer)
2. **Windows/macOS**: Run the installer, follow the prompts (npm is included)
3. **Linux (Ubuntu/Debian)**:
    ```bash
    curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
    sudo apt-get install -y nodejs
    ```
4. **Verify installation**:
    ```bash
    node --version    # should print v18.x.x or higher
    npm --version     # should print 9.x.x or higher
    ```

> **Alternative**: Use [nvm (Node Version Manager)](https://github.com/nvm-sh/nvm) to install and switch between Node.js versions easily:
>
> ```bash
> curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash
> nvm install --lts
> ```

### VS Code Installation

1. **Download** from [https://code.visualstudio.com](https://code.visualstudio.com)
2. **Windows**: Run the `.exe` installer
3. **macOS**: Drag the `.app` to Applications
4. **Linux (Ubuntu/Debian)**:
    ```bash
    sudo snap install code --classic
    ```
    Or download the `.deb` package from the website and install with `sudo dpkg -i code_*.deb`

**Recommended VS Code Extensions** (install from the Extensions panel — `Ctrl+Shift+X`):

| Extension           | ID                                | Purpose                       |
| ------------------- | --------------------------------- | ----------------------------- |
| Arduino             | `vsciot-vscode.vscode-arduino`    | Arduino & ESP32 development   |
| ESLint              | `dbaeumer.vscode-eslint`          | JavaScript/TypeScript linting |
| Prettier            | `esbenp.prettier-vscode`          | Code formatting               |
| ES7+ React Snippets | `dsznajder.es7-react-js-snippets` | React boilerplate shortcuts   |

### Hardware

Same wiring as P4-Interrupt (ESP32-CAM + Arduino UNO + motors + HC-SR04).

---

## Setup & Run

### 1. Flash Arduino

1. Open `hardware/arduino/main.ino` in Arduino IDE
2. Board: **Tools → Board → Arduino AVR Boards → Arduino Uno**
3. Port: **Tools → Port →** select your Arduino's port (e.g. `/dev/ttyUSB1`)
4. Upload (Ctrl+U)
5. Open Serial Monitor (Ctrl+Shift+M) at **9600 baud** → should see `ARDUINO_READY`

### 2. Flash ESP32

1. Open `hardware/esp32/main.ino` in Arduino IDE
2. Board: **Tools → Board → ESP32 Arduino → AI Thinker ESP32-CAM**
    > If you don't see ESP32 boards, add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` in **File → Preferences → Additional Board Manager URLs**, then install **esp32** from **Tools → Board Manager**
3. Update WiFi credentials (`ssid` and `password`) in `main.ino`
4. Port: **Tools → Port →** select your ESP32's port (e.g. `/dev/ttyUSB0`)
5. Upload (Ctrl+U)
6. Open Serial Monitor at **9600 baud** → note the IP address printed

### 3. Start Backend

```bash
cd backend
cp .env.example .env        # edit ESP32_IP to match your device
npm install
npm run dev                  # starts on http://localhost:3001
```

### 4. Start Frontend

```bash
cd frontend
cp .env.example .env         # edit stream URL if needed
npm install
npm run dev                  # opens http://localhost:5173
```

---

## ESP32 API Endpoints

| Method  | Path           | Description                                     |
| ------- | -------------- | ----------------------------------------------- |
| GET     | /api/telemetry | `{ distance, motorState, connected, uptimeMs }` |
| GET     | /api/status    | `{ ssid, ip, freeHeap, uptimeMs }`              |
| GET     | /command?cmd=X | P4-compatible motor command (F/B/L/R/S)         |
| POST    | /api/command   | `{ "cmd": "F" }` — preferred by backend         |
| OPTIONS | /api/command   | CORS preflight                                  |
| GET     | :81/stream     | MJPEG camera stream                             |

All endpoints return `Access-Control-Allow-Origin: *`.

---

## Dashboard Features

- **Camera Feed** — Embedded MJPEG stream directly from ESP32 port 81
- **Live Distance Chart** — Recharts line graph showing the last 60 readings with colored danger zones (red ≤10 cm, orange ≤30 cm, yellow ≤100 cm)
- **Motor Control Panel** — 5-button grid (Forward, Backward, Left, Right, Stop) with active state highlighting
- **Status Bar** — Backend connection status, ESP32 connection status, color-coded distance alert

---

## WebSocket Protocol

The Express server rebroadcasts ESP32 telemetry to all connected dashboard clients.

### Server → Client

```json
{
	"type": "telemetry",
	"distance": 42,
	"motorState": "F",
	"connected": true,
	"uptimeMs": 123456,
	"timestamp": 1706367600000
}
```

### Client → Server

```json
{ "type": "command", "cmd": "F" }
```

The server forwards the command to ESP32 via `POST /api/command` and broadcasts an acknowledgment:

```json
{ "type": "command_ack", "cmd": "F" }
```

---

## Enhancement Challenges

These are left as exercises for participants to extend the project:

1. **Database Integration** — Add SQLite or MongoDB to persist telemetry history, then query historical trends in the dashboard
2. **Multi-Device Support** — Extend the backend to track multiple ESP32 devices, each with its own telemetry stream
3. **Authentication** — Add JWT-based login to protect the dashboard API
4. **Docker Deployment** — Containerize the backend and frontend for production deployment
5. **Speed Slider** — Add a range input that sends a PWM value alongside the direction command

---

## Comparison: ESP32 Web UI vs. Dashboard

| Aspect            | P4 (ESP32 serves HTML) | P5 (Express + React)    |
| ----------------- | ---------------------- | ----------------------- |
| UI complexity     | Limited (embedded JS)  | Rich (React + Recharts) |
| Maintainability   | Hard (strings in C++)  | Clean (JSX + CSS)       |
| Scalability       | 1 client at a time     | Many clients via WS     |
| Data persistence  | None                   | Extensible (add DB)     |
| Development speed | Slow (reflash cycle)   | Fast (Vite hot reload)  |
| ESP32 memory      | High (HTML in flash)   | Low (API only)          |

---

## Troubleshooting

| Issue                     | Solution                                                          |
| ------------------------- | ----------------------------------------------------------------- |
| Dashboard shows "offline" | Check ESP32 IP in backend `.env`, ensure same WiFi network        |
| Camera feed not loading   | Verify ESP32 port 81 is reachable; check CORS in browser DevTools |
| WebSocket disconnects     | Auto-reconnects in 3s — check backend logs for errors             |
| "CORS error" in console   | ESP32 `app_httpd.cpp` sets `Access-Control-Allow-Origin: *`       |
| Distance reads 0          | Check Arduino wiring (Trig/Echo pins) and serial baud rate        |

---

## Previous Module

← [P4-Interrupt](../P4-Interrupt/) — Master-slave serial protocol

## Next Module

→ [P6-Gesture_HTTP](../P6-Gesture_HTTP/) — Hand gesture control via HTTP
