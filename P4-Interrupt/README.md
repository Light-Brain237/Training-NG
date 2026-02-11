# ESP32-CAM Robot Control System

A unified robot control system combining ESP32-CAM for WiFi/camera streaming, Arduino UNO for motor control, and HC-SR04 ultrasonic sensor for distance measurement.

## 🎯 System Overview

**Architecture**: ESP32-CAM (Master) ↔ Serial Communication ↔ Arduino UNO (Slave)

- **ESP32-CAM**: WiFi web server, camera streaming, FreeRTOS multitasking, Serial master
- **Arduino UNO**: Motor control via 74HCT595N shift register, HC-SR04 distance sensor, Serial slave
- **Communication**: Master-Slave protocol over Serial (9600 baud) prevents TX collisions
- **Web Interface**: Virtual joystick control, live camera feed, real-time distance monitoring, system status indicator

## 📋 Hardware Requirements

### ESP32-CAM (AI Thinker)
- ESP32 dual-core @ 240MHz
- OV2640 camera module
- WiFi connectivity
- Serial: TX (GPIO1), RX (GPIO3)

### Arduino UNO
- ATmega328P microcontroller
- 74HCT595N shift register (motor control)
- HC-SR04 ultrasonic sensor
  - Trig: Pin 12
  - Echo: Pin 13
- Serial: TX (Pin 1), RX (Pin 0)

### Connections
```
ESP32-CAM TX (GPIO1) ──→ Arduino RX (Pin 0)
ESP32-CAM RX (GPIO3) ──→ Arduino TX (Pin 1)
ESP32-CAM GND ──────────→ Arduino GND
```

## 🚀 Features

### 1. Virtual Joystick Control
- 8-directional movement (Forward, Backward, Left, Right, + Diagonals)
- Color-coded visual feedback (9 colors for different directions)
- Auto-stop on release
- 200px diameter, 60px max travel distance
- 15px dead zone for precision

### 2. Live Camera Streaming
- Real-time video feed on port 81
- MJPEG streaming
- Embedded in web interface

### 3. Distance Monitoring
- Real-time HC-SR04 readings every 500ms
- Visual proximity bar with color gradients
- Safety status indicators:
  - **< 10cm**: Red - VERY CLOSE - DANGER!
  - **< 30cm**: Orange - Close - Be Careful
  - **< 100cm**: Yellow - Safe Distance
  - **≥ 100cm**: Green - Clear Path Ahead
- Forward movement blocked if obstacle < 15cm

### 4. System Status Indicator
- **Green pulsing dot**: Arduino connected and responding
- **Red pulsing dot**: Arduino disconnected or no Serial response
- Real-time connection monitoring (checks every 2 seconds)
- Detects Serial cable disconnect, Arduino power loss, or communication failure

### 5. LED Control
- Flash LED on/off buttons
- Manual light control for visibility

## 🔧 Software Architecture

### Master-Slave Protocol
Prevents Serial communication collisions in half-duplex mode:

**ESP32 (Master) Commands:**
```
GET_DISTANCE\n          → Request distance from sensor
F\n                     → Forward
B\n                     → Backward
L\n                     → Turn Left
R\n                     → Turn Right
A\n                     → Forward Left
D\n                     → Forward Right
Q\n                     → Backward Left
W\n                     → Backward Right
Z\n                     → Rotate Left
X\n                     → Rotate Right
S\n                     → Stop
```

**Arduino (Slave) Responses:**
```
DISTANCE:XX\n           → Distance value (XX = cm)
ACK:F\n                 → Command acknowledged and executed
```

### FreeRTOS Multitasking (ESP32)
- **Core 0**: `serialTask()` - Handles Serial communication without blocking
  - Priority: 3
  - Requests distance every 500ms
  - Sends motor commands via queue
- **Core 1**: Main loop - WiFi, HTTP server, camera streaming
- **Mutex**: `serialMutex` prevents concurrent Serial access from both cores
- **Queue**: `distanceQueue` for thread-safe data sharing (size: 5)

### Connection Monitoring
- Tracks last Arduino response timestamp (`lastArduinoResponse`)
- Status check: If response within last 2 seconds → Connected
- `/system_status` endpoint returns `{"system_connected": true/false}`
- Webpage polls status every 2 seconds
- Automatic status indicator update (green/red dot)

## 📁 Project Structure

```
src/interrupt/
├── arduino/
│   └── main.cpp          # Arduino slave code (motor + sensor)
├── esp32/
│   ├── main.cpp          # ESP32 master code (FreeRTOS + Serial)
│   ├── app_httpd.cpp     # Web server + embedded HTML
│   └── unified_interface.html  # Full reference HTML (development)
└── README.md             # This file
```

## ⚙️ Configuration

### WiFi Settings (ESP32)
Edit `src/interrupt/esp32/main.cpp`:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### Serial Baud Rate
Both devices use **9600 baud** (configurable in both main.cpp files)

### Motor Speeds
Edit `src/interrupt/arduino/main.cpp`:
```cpp
Motor(250, 250);  // Speed values 0-255
```

### Distance Request Interval
Edit `src/interrupt/esp32/main.cpp`:
```cpp
vTaskDelay(500 / portTICK_PERIOD_MS);  // 500ms between requests
```

## 🔨 Build & Upload

### Using PlatformIO

**Upload Arduino Code:**
```bash
pio run -e interrupt_arduino --target upload --upload-port /dev/ttyUSB1
```

**Upload ESP32 Code:**
```bash
pio run -e interrupt_esp32 --target upload --upload-port /dev/ttyUSB0
```

**Monitor ESP32 Serial (to get IP address):**
```bash
pio device monitor --port /dev/ttyUSB0 --baud 9600
```

### PlatformIO Environments
Defined in `platformio.ini`:

**interrupt_arduino:**
- Platform: atmelavr
- Board: uno
- Upload speed: 9600
- Build flags: `-DARDUINO_ARCH_AVR`

**interrupt_esp32:**
- Platform: espressif32
- Board: esp32cam
- Upload speed: 115200
- Partition: huge_app.csv (3MB)
- Build flags: Camera pins for AI Thinker model

## 🌐 Web Interface

### Access
1. Power on both Arduino and ESP32-CAM
2. Open Serial Monitor to see ESP32's IP address
3. Navigate to: `http://[ESP32_IP]` in your browser

### Features
- **Camera Stream**: Top section, auto-loads on page load
- **Virtual Joystick**: Center control, drag to move robot
- **Distance Display**: Real-time sensor reading with proximity bar
- **Status Indicator**: Green/red dot shows Arduino connection state
- **Control Buttons**: 
  - Turn Left / Turn Right (rotation in place)
  - Stop (emergency stop)
  - Light On / Light Off

### Endpoints
- `http://[ESP32_IP]/` - Main interface
- `http://[ESP32_IP]:81/stream` - Camera stream
- `http://[ESP32_IP]/distance` - Distance JSON
- `http://[ESP32_IP]/system_status` - Connection status JSON
- `http://[ESP32_IP]/[F|B|L|R|A|D|Q|W|Z|X|S]` - Motor commands
- `http://[ESP32_IP]/ledon` - Flash LED on
- `http://[ESP32_IP]/ledoff` - Flash LED off

## 🐛 Troubleshooting

### ESP32 not connecting to WiFi
- Check SSID/password in `main.cpp`
- Verify WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
- Check Serial Monitor for connection errors

### Arduino not responding (Red status dot)
- Check Serial connections (TX ↔ RX, GND)
- Verify both devices share common ground
- Check Arduino is powered and programmed
- Monitor Serial traffic at 9600 baud

### Distance always shows 0 or 999
- Check HC-SR04 connections (Trig: Pin 12, Echo: Pin 13)
- Ensure sensor has clear line of sight
- Test sensor independently on Arduino

### Motor not responding
- Verify 74HCT595N shift register connections
- Check motor power supply (separate from logic)
- Test motor commands via Serial Monitor first

### Camera not streaming
- Check camera cable connection to ESP32-CAM
- Verify camera is enabled in code (AI Thinker model)
- Check port 81 is accessible (firewall rules)

### Status indicator stuck on "Connecting..."
- Clear browser cache and reload page
- Check `/system_status` endpoint manually: `http://[ESP32_IP]/system_status`
- Verify FreeRTOS task is running (Serial Monitor shows "GET_DISTANCE" requests)

## 📊 Memory Usage

### Arduino UNO
- Flash: 5,708 bytes (17.7% of 32,256 bytes)
- RAM: 246 bytes (12.0% of 2,048 bytes)

### ESP32-CAM
- Flash: 866,137 bytes (27.5% of 3,145,728 bytes)
- RAM: 56,100 bytes (17.1% of 327,680 bytes)

## 🔐 Security Notes

- Web interface has no authentication (add if deploying publicly)
- WiFi password stored in plain text in source code
- Local network access recommended

## 📝 License

This project is for educational and personal use.

## 🤝 Contributing

Feel free to fork and improve! Suggested enhancements:
- Add speed control slider
- Implement autonomous navigation mode
- Add WebSocket for real-time bidirectional communication
- Create mobile-responsive interface
- Add camera pan/tilt servo control

## 📧 Support

For issues or questions, check:
1. Serial Monitor output (both devices)
2. Browser Console (F12) for JavaScript errors
3. `/system_status` endpoint for connection state
4. This README's Troubleshooting section

---

**Version**: 1.0  
**Date**: December 2, 2025  
**Tested on**: Arduino UNO R3, ESP32-CAM AI Thinker, PlatformIO Core 6.x
