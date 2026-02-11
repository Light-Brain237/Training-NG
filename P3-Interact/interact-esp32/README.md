# ESP32-CAM Web-Based Motor Control Server

## Overview
This ESP32-CAM program creates a WiFi web server that provides a browser-based interface for controlling DC motors. Users control motor speed via a slider (0-100%) and direction via buttons. The ESP32 sends commands to an Arduino via Serial communication.

## Hardware Requirements

### Components
- ESP32-CAM (AI Thinker model)
- FTDI programmer or USB-to-Serial adapter
- Arduino UNO (receives commands)
- WiFi network (2.4GHz)

### Connections
- **Serial Communication**: ESP32 TX → Arduino RX, ESP32 RX → Arduino TX
- **Power**: 5V via USB or external power supply
- **Programming**: FTDI adapter for initial upload

## WiFi Configuration

### WiFi Credentials (in main.cpp)
```cpp
const char* ssid = "TP-LINK_4E81";      // Your WiFi SSID
const char* password = "Jk123456789";    // Your WiFi password
```

**⚠️ Important**: Update these values with your WiFi credentials before uploading!

## Project Structure

### Files
```
interact-esp32/
├── main.cpp           - WiFi setup, Serial init, web server startup
├── app_httpd.cpp      - HTTP server, endpoint handlers, Serial commands
├── motor_control.h    - HTML web interface (stored in PROGMEM)
└── README.md         - This file
```

### File Descriptions

**main.cpp**
- Initializes Serial communication (9600 baud)
- Connects to WiFi network
- Starts web server
- Displays IP address

**app_httpd.cpp**
- Configures HTTP server on port 80
- Handles POST requests for speed/direction
- Handles GET requests for status/page
- Sends commands to Arduino via Serial

**motor_control.h**
- Complete HTML/CSS/JavaScript web page
- Stored in PROGMEM (flash memory)
- Beautiful gradient design
- Responsive layout

## Web Interface

### Features
✅ **Speed Slider** - Smooth 0-100% control  
✅ **Direction Buttons** - Forward, Backward, Stop  
✅ **Real-time Display** - Current speed and direction  
✅ **Status Indicator** - Connection status  
✅ **Auto-refresh** - Status updates every 2 seconds  
✅ **Responsive Design** - Works on phone, tablet, PC

### User Interface Elements

**Status Bar**
- Current Speed: 0-100%
- Direction: FORWARD/BACKWARD/STOPPED (color-coded)

**Speed Control**
- Large slider with visual feedback
- Real-time percentage display
- Smooth drag interaction
- Updates on release

**Direction Controls**
- 🔺 Forward (Green)
- 🔻 Backward (Orange)
- ⏹ Stop (Red)

**Connection Status**
- ● Connected to ESP32 (Green)
- ● Connection Error (Red)

## HTTP Endpoints

### GET `/`
**Description**: Serves the main HTML page  
**Response**: HTML content  
**Content-Type**: `text/html`

### POST `/speed`
**Description**: Set motor speed  
**Body**: `"0"` to `"100"` (string)  
**Response**: JSON
```json
{
  "speed": 75,    // Percentage (0-100)
  "pwm": 191      // PWM value (0-255)
}
```
**Action**: Sends `SPEED:191\n` to Arduino

### POST `/direction`
**Description**: Set motor direction  
**Body**: `"F"`, `"B"`, or `"S"` (string)  
**Response**: JSON
```json
{
  "direction": "FORWARD"
}
```
**Action**: Sends `DIR:F\n` to Arduino

### GET `/status`
**Description**: Get current motor state  
**Response**: JSON
```json
{
  "speed": 65,
  "pwm": 166,
  "direction": "FORWARD"
}
```

## Serial Communication

### Settings
- **Baud Rate**: 9600
- **Format**: 8N1
- **Connection**: ESP32 TX → Arduino RX

### Command Format

#### Speed Commands
```
SPEED:XXX\n
```
- Converts slider value (0-100) to PWM (0-255)
- Example: Slider at 75% → `SPEED:191\n`

#### Direction Commands
```
DIR:F\n   - Forward
DIR:B\n   - Backward
DIR:S\n   - Stop
```

### Conversion Logic
```cpp
// Slider percentage to PWM
int speedPercent = 75;                      // From slider
currentSpeed = (speedPercent * 255) / 100;  // = 191 PWM

// Send to Arduino
Serial.println("SPEED:191");
```

## How It Works

### System Flow
```
User Browser
    ↓ HTTP Request
ESP32 Web Server
    ↓ Parse Request
Convert % to PWM
    ↓ Serial TX
Arduino
    ↓ Motor Control
DC Motors
```

### Detailed Sequence

1. **User moves slider to 75%**
   - JavaScript sends POST to `/speed` with body `"75"`

2. **ESP32 receives request**
   - `speed_handler()` parses value
   - Converts: `(75 * 255) / 100 = 191`
   - Updates `currentSpeed = 191`

3. **ESP32 sends to Arduino**
   - `sendToArduino("SPEED:191")`
   - Sends: `"SPEED:191\n"` via Serial TX

4. **Arduino receives and applies**
   - Parses command
   - Sets PWM pins to 191
   - Motors run at 75% speed

5. **Web page updates**
   - JavaScript displays "75%"
   - Status bar shows current speed

## Startup Sequence

### Boot Process
1. **Serial Init** (9600 baud)
2. **WiFi Connection**
   - Attempts to connect
   - Max 20 attempts (10 seconds)
   - Displays connection status
3. **Web Server Start**
   - Binds to port 80
   - Registers all endpoints
4. **Ready**
   - Displays IP address
   - Waits for browser connections

### Serial Output Example
```
========================================
ESP32 MOTOR CONTROL WEB SERVER
========================================
Connecting to WiFi: TP-LINK_4E81
.....
[SUCCESS] WiFi connected!
IP Address: 192.168.1.102

Starting web server...
[SUCCESS] Web server started!

========================================
READY!
Open browser: http://192.168.1.102
========================================
```

## Memory Usage
- **RAM**: 43,544 bytes (13.3% of 320KB)
- **Flash**: 782,497 bytes (24.9% of 3MB)
- HTML page stored in PROGMEM (saves RAM)

## Compile & Upload (Arduino IDE)

1. **Install ESP32 Board Support**
   - Arduino IDE: `File > Preferences > Additional Boards Manager URLs`
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Then: `Tools > Board > Boards Manager` → install **esp32** by Espressif

2. **Open the Sketch Folder**
   - Create/open a folder containing these files in the same directory:
     - `main.ino` (use the content from `main.cpp`)
     - `app_httpd.cpp`
     - `motor_control.h`

3. **Select Board & Port**
   - Board: `Tools > Board > ESP32 Arduino > AI Thinker ESP32-CAM`
   - Port: `Tools > Port > /dev/ttyUSB0`
   - Upload Speed: `Tools > Upload Speed > 921600` (or 115200 if unstable)

4. **Compile & Upload**
   - `Sketch > Verify/Compile` (Ctrl+R)
   - `Sketch > Upload` (Ctrl+U)
   - If you see “Connecting...”, hold **IO0** during upload

5. **Serial Monitor**
   - `Tools > Serial Monitor` (Ctrl+Shift+M)
   - Set baud rate to **9600** (matches `Serial.begin(9600)` in `main.cpp`)

## Web Page Design

### Color Scheme
- **Background**: Purple gradient (#667eea → #764ba2)
- **Container**: White with shadow
- **Slider**: Purple (#667eea)
- **Forward Button**: Green (#4CAF50)
- **Backward Button**: Orange (#FF9800)
- **Stop Button**: Red (#f44336)

### Responsive Design
- Desktop: Full-width container (max 500px)
- Tablet: Optimized button sizes
- Mobile: Compact layout, larger touch targets

### JavaScript Features
- Real-time slider updates
- AJAX requests (fetch API)
- Error handling
- Auto-refresh status (2s interval)
- Connection monitoring

## Troubleshooting

### WiFi Connection Issues
**Problem**: ESP32 can't connect to WiFi

**Solutions**:
1. Check SSID and password in `main.cpp`
2. Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
3. Check WiFi signal strength
4. Verify router is powered on
5. Check for special characters in password

### Can't Access Web Page
**Problem**: Browser can't load page

**Solutions**:
1. Check ESP32 serial monitor for IP address
2. Ensure computer is on same WiFi network
3. Try `http://` prefix: `http://192.168.1.102`
4. Check firewall settings
5. Ping ESP32 IP to verify network connection

### Commands Not Reaching Arduino
**Problem**: Motors don't respond to web controls

**Solutions**:
1. Verify Serial wiring (TX→RX, RX→TX)
2. Check baud rate matches (9600)
3. Open Arduino serial monitor to see commands
4. Verify ground connection between boards
5. Check ESP32 is sending (add debug output)

### Web Page Loads But Controls Don't Work
**Problem**: Page appears but buttons/slider inactive

**Solutions**:
1. Check browser console for JavaScript errors (F12)
2. Verify endpoints are registered
3. Test endpoints manually with curl:
   ```bash
   curl -X POST http://192.168.1.102/speed -d "50"
   curl -X POST http://192.168.1.102/direction -d "F"
   ```
4. Check ESP32 serial for request logs

## Testing

### Manual Testing
1. **Upload both codes** (ESP32 and Arduino)
2. **Open ESP32 serial monitor** at 115200 baud
3. **Note the IP address** displayed
4. **Open browser** to that IP
5. **Test slider** - watch Arduino serial for commands
6. **Test buttons** - observe motor behavior

### Network Testing
```bash
# Ping ESP32
ping 192.168.1.102

# Test speed endpoint
curl -X POST http://192.168.1.102/speed -d "75"

# Test direction endpoint
curl -X POST http://192.168.1.102/direction -d "F"

# Get status
curl http://192.168.1.102/status
```

## Security Considerations

### Current Implementation
⚠️ **No authentication** - Anyone on WiFi can control motors  
⚠️ **No encryption** - HTTP only (not HTTPS)  
⚠️ **Open network** - No access restrictions

### Recommended for Production
- Add password protection
- Implement HTTPS
- Use access control lists
- Add request rate limiting
- Log all commands

## Performance

### Response Times
- **WiFi connection**: ~5 seconds
- **Page load**: ~500ms
- **Slider update**: <100ms
- **Button press**: <50ms
- **Status refresh**: 2 seconds (configurable)

### Network Requirements
- **Bandwidth**: <10 KB/s
- **Latency**: <100ms recommended
- **WiFi**: 2.4GHz 802.11 b/g/n

## Future Enhancements

Possible improvements:
- Add camera streaming integration
- Implement WebSocket for real-time updates
- Add user authentication
- Create mobile app
- Add speed presets
- Include battery voltage monitoring
- Add obstacle detection display
- Multi-language support
- Night mode theme

## Related Files
- **Arduino Code**: `../arduino/main.cpp` (Receives and executes commands)
- **PlatformIO Config**: `../../../platformio.ini` (Build configuration)

## API Reference

### JavaScript Functions (in motor_control.h)

**`sendSpeed(speed)`**
- Sends speed to ESP32
- Parameters: speed (0-100)
- Updates display on success

**`setDirection(dir)`**
- Sends direction command
- Parameters: dir ('F', 'B', or 'S')
- Updates status indicator

**`updateStatus()`**
- Fetches current state from `/status`
- Called every 2 seconds
- Updates all display elements

**`updateDirection(direction)`**
- Updates direction indicator
- Changes color based on state
- Parameters: direction string

## Dependencies
- **ESP32 Arduino Core**: v2.0+
- **WiFi Library**: Built-in
- **HTTP Server**: Built-in (esp_http_server)

---
**Version**: 1.0  
**Date**: December 2025  
**Platform**: PlatformIO + Arduino Framework  
**Board**: ESP32-CAM (AI Thinker)  
**Author**: Trial_1 Project
