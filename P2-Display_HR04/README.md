# HC-SR04 Distance Display System

## Overview
This project displays real-time distance measurements from an HC-SR04 ultrasonic sensor on a web interface. The Arduino reads the sensor and sends data to an ESP32-CAM via Serial, which hosts a web server that displays the distance with color-coded proximity indicators.

## System Architecture

```
HC-SR04 Sensor (Pins 12/13)
    ↓
Arduino UNO (reads sensor)
    ↓ Serial @ 9600 baud
ESP32-CAM (receives data)
    ↓ WiFi
Web Browser
    ↓
User sees real-time distance
```

## Hardware Requirements

### Components
- **Arduino UNO** (ATmega328P)
- **ESP32-CAM** (AI Thinker)
- **HC-SR04 Ultrasonic Sensor**
- **USB cables** for programming

### Connections

#### Arduino to HC-SR04
```
HC-SR04 VCC  → Arduino 5V
HC-SR04 GND  → Arduino GND
HC-SR04 TRIG → Arduino Pin 12
HC-SR04 ECHO → Arduino Pin 13
```

#### Arduino to ESP32-CAM
```
Arduino TX → ESP32 RX
Arduino RX → ESP32 TX
GND → GND
```

## Software Components

### Arduino Code (`/src/display-HR04/arduino/`)
**Purpose**: Read HC-SR04 sensor and send distance data to ESP32

**File Structure**:
```
arduino/
└── main.cpp              - Sensor reading and Serial transmission
```

**Key Features**:
- Measures distance using ultrasonic pulse timing
- Sends formatted data: `DISTANCE:XX\n`
- Update rate: 500ms (2 readings per second)

**Main Function**:
- `SR04()` - Measures distance using ultrasonic sensor timing

### ESP32 Code (`/src/display-HR04/esp32/`)
**Purpose**: WiFi web server that receives distance data and displays it

**File Structure**:
```
esp32/
├── main.cpp              - WiFi setup, Serial init, web server startup
├── app_httpd.cpp         - HTTP endpoints, Serial data processing
├── distance_display.h    - HTML/CSS/JS stored in PROGMEM
└── distance_display.html - Reference HTML file
```

**Key Features**:
- Connects to WiFi network
- Receives distance data via Serial (9600 baud)
- Serves web interface with real-time updates
- Provides HTTP endpoints for data access

## Communication Protocol

### Serial Format (Arduino → ESP32)
```
DISTANCE:25\n     - Valid reading (25 cm)
DISTANCE:142\n    - Valid reading (142 cm)
```

### HTTP Endpoints (ESP32)

#### GET `/`
Returns the main HTML page with distance display interface

#### GET `/distance`
Returns current distance data in JSON format
```json
{
  "distance": 25,
  "unit": "cm",
  "valid": true
}
```

#### GET `/status`
Returns system status and uptime
```json
{
  "distance": 25,
  "valid": true,
  "uptime": 3600,
  "lastUpdate": 2
}
```

## Web Interface Features

### Visual Elements
- **Large Distance Display** - Shows current distance in centimeters
- **Proximity Bar** - Visual indicator that fills based on distance
- **Color-Coded Status**:
  - 🔴 **RED** (< 10 cm) - VERY CLOSE
  - 🟠 **ORANGE** (10-30 cm) - CLOSE
  - 🟡 **YELLOW** (30-100 cm) - MEDIUM
  - 🟢 **GREEN** (> 100 cm) - FAR
- **Connection Indicator** - Shows if ESP32 is connected
- **Sensor Specifications** - Displays range (2-400 cm) and update rate (500 ms)

### Real-Time Updates
- JavaScript fetches distance every 1 second
- Smooth transitions between color states
- Automatic error handling and recovery
- No page refresh required

## WiFi Configuration

### Network Credentials (in both main.cpp files)
```cpp
const char* ssid = "TP-LINK_4E81";
const char* password = "Jk123456789";
```

⚠️ **Important**: Update these values with your WiFi credentials before uploading!

## Compilation & Upload

### Using PlatformIO

#### 1. Upload Arduino Code
```bash
# Compile and upload to Arduino UNO
platformio run -e display_arduino --target upload --upload-port /dev/ttyUSB1

# Monitor Arduino serial output
platformio device monitor --port /dev/ttyUSB1 --baud 9600
```

#### 2. Upload ESP32 Code
```bash
# Compile and upload to ESP32-CAM
platformio run -e display_esp32 --target upload --upload-port /dev/ttyUSB0

# Monitor ESP32 serial output
platformio device monitor --port /dev/ttyUSB0 --baud 9600
```

### Environment Configuration (platformio.ini)
```ini
[env:display_arduino]
platform = atmelavr
board = uno
framework = arduino
monitor_speed = 9600
build_src_filter = +<display-HR04/arduino/*>

[env:display_esp32]
platform = espressif32
board = esp32cam
framework = arduino
monitor_speed = 115200
build_src_filter = +<display-HR04/esp32/*>
```

## How to Use

### Step 1: Upload Code
1. Connect Arduino UNO to USB port (e.g., /dev/ttyUSB1)
2. Upload Arduino code
3. Connect ESP32-CAM to USB port (e.g., /dev/ttyUSB0)
4. Upload ESP32 code

### Step 2: Find IP Address
1. Open ESP32 serial monitor at 9600 baud
2. Press RESET button on ESP32-CAM
3. Look for WiFi connection messages:
   ```
   .....
   WiFi connected
   Distance Display Ready! Use 'http://192.168.1.XXX' to connect
   ```
4. Note the IP address (e.g., 192.168.1.102)

### Step 3: View Web Interface
1. Open web browser
2. Navigate to: `http://192.168.1.102` (use your IP)
3. See real-time distance measurements
4. Move objects near the sensor to see updates

### Step 4: Monitor Data Flow
**Arduino Serial Output:**
```
========================================
Robot Car - Distance Display Mode
========================================
Sending distance data to ESP32...
========================================

DISTANCE:25
DISTANCE:26
DISTANCE:28
```

**ESP32 Serial Output:**
```
.....
WiFi connected
Distance Display Ready! Use 'http://192.168.1.102' to connect
[ESP32] Distance: 25.0 cm
[ESP32] Distance: 26.0 cm
```

## Memory Usage

### Arduino
- **Flash**: 3,414 bytes (10.6% of 32KB)
- **RAM**: 352 bytes (17.2% of 2KB)

### ESP32
- **Flash**: 787,905 bytes (25.0% of 3MB)
- **RAM**: 43,596 bytes (13.3% of 320KB)

## Troubleshooting

### Arduino Issues

**Problem**: No distance readings
- Check HC-SR04 power (5V, GND)
- Verify Trig pin 12, Echo pin 13
- Ensure sensor is facing open space
- Check serial monitor for "DISTANCE:XX" messages

**Problem**: Erratic readings
- Sensor may be too close to object (< 2 cm)
- Object may be beyond range (> 400 cm)
- Check for loose wiring
- Ensure stable 5V power supply

### ESP32 Issues

**Problem**: Can't connect to WiFi
- Verify SSID and password in main.cpp
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check WiFi signal strength
- Look for dots (`.....`) in serial monitor

**Problem**: Garbled serial output
- Garbled text at boot is normal (115200 baud boot messages)
- After boot, output switches to 9600 baud
- Press RESET to see connection messages again

**Problem**: Web page doesn't load
- Verify IP address from serial monitor
- Ensure computer is on same WiFi network
- Try ping: `ping 192.168.1.102`
- Check firewall settings
- Use `http://` prefix, not `https://`

### Communication Issues

**Problem**: Web shows "No Data" or "Connection Error"
- Check TX/RX wiring between Arduino and ESP32
- Verify both use 9600 baud rate
- Ensure common ground connection
- Check Arduino is sending DISTANCE:XX format
- Monitor both serial ports simultaneously

**Problem**: Distance not updating
- Arduino may not be reading sensor
- Check sensor power and connections
- Verify Arduino serial is sending data
- Check ESP32 is receiving (look for [ESP32] messages)

## Technical Details

### Distance Measurement Algorithm
The HC-SR04 uses ultrasonic pulses to measure distance:

1. Arduino sends 15µs trigger pulse
2. Sensor emits 8-cycle ultrasonic burst at 40kHz
3. Sound wave travels to object and reflects back
4. Arduino measures echo pulse duration
5. Distance calculated: `distance = duration / 58.8`
   - Speed of sound: 343 m/s
   - Formula: `distance (cm) = (time × 343) / 2 / 10000`
   - Simplified: `distance = time / 58.8`

### Data Flow Timeline
```
0ms:    Arduino triggers HC-SR04
~1ms:   Echo received, distance calculated
0ms:    Arduino sends "DISTANCE:XX\n"
~1ms:   ESP32 receives and parses data
0ms:    ESP32 stores currentDistance value
1000ms: Browser fetches /distance endpoint
~100ms: ESP32 responds with JSON
0ms:    JavaScript updates display and colors
```

### Web Page Technology
- **HTML5** - Structure and semantic markup
- **CSS3** - Gradients, animations, responsive design
- **JavaScript** - Fetch API, real-time updates, DOM manipulation
- **No Libraries** - Pure vanilla JS, no jQuery or frameworks

## Future Enhancements

Possible improvements:
- [ ] Add distance history graph
- [ ] Implement WebSocket for real-time push updates
- [ ] Add multiple sensor support
- [ ] Create distance alert thresholds
- [ ] Log data to SD card
- [ ] Add temperature compensation
- [ ] Mobile app integration
- [ ] Export data to CSV
- [ ] Add min/max distance tracking
- [ ] Implement distance averaging filter

## Performance Characteristics

### Accuracy
- **HC-SR04 Accuracy**: ±3mm
- **Effective Range**: 2 cm to 400 cm
- **Measurement Angle**: 15 degrees cone
- **Resolution**: 0.3 cm

### Response Times
- **Sensor Reading**: ~10ms per measurement
- **Arduino Processing**: <1ms
- **Serial Transmission**: ~1ms (9600 baud)
- **Web Update Interval**: 1 second (configurable)
- **Total Latency**: <50ms from sensor to ESP32

### Network Performance
- **WiFi Latency**: <100ms typical
- **HTTP Response**: <50ms
- **Page Load**: <500ms
- **Bandwidth**: <1 KB/s

## Safety Considerations

### Hardware Safety
- ⚠️ Use proper voltage levels (5V for Arduino, 3.3V for ESP32 logic)
- ⚠️ Avoid short circuits
- ⚠️ Ensure proper power supply capacity

### Network Security
- ⚠️ **No authentication** - Anyone on WiFi can access
- ⚠️ **No encryption** - HTTP only, not HTTPS
- ⚠️ Recommended for local/private networks only
- Consider adding password protection for production use

## Related Projects

This project is part of the `trial_1` workspace and is a standalone distance measurement display system.

## Dependencies

### Arduino Libraries
- **Arduino.h** - Core Arduino framework

### ESP32 Libraries
- **Arduino.h** - Core ESP32 Arduino framework
- **WiFi.h** - WiFi connectivity
- **esp_http_server.h** - HTTP server functionality

All libraries are included with PlatformIO ESP32 platform.

## License & Credits

**Version**: 1.0  
**Date**: December 2025  
**Platform**: PlatformIO + Arduino Framework  
**Boards**: Arduino UNO + ESP32-CAM (AI Thinker)

## Support

For issues or questions:
1. Check serial monitor outputs for both boards
2. Verify all connections
3. Ensure WiFi credentials are correct
4. Check that both boards are powered properly
5. Review the troubleshooting section above

---

**Ready to measure distances! 🎯📏**

Access your distance display at: `http://[YOUR-ESP32-IP]`
