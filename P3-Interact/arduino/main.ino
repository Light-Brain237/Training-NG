// ============================================
// WEB-BASED MOTOR SPEED CONTROL - ARDUINO
// ============================================
// Receives commands from ESP32-CAM via Serial
// Commands format:
//   SPEED:XXX  (XXX = 0-255 PWM value)
//   DIR:F      (Forward)
//   DIR:B      (Backward)
//   DIR:S      (Stop)
// ============================================

// PWM control pins
#define PWM1_PIN            5
#define PWM2_PIN            6

// 74HCT595N shift register pins
#define SHCP_PIN            2       // Shift register clock pin
#define EN_PIN              7       // Enable pin
#define DATA_PIN            8       // Serial data pin
#define STCP_PIN            4       // Storage register clock pin

// Motor direction values
const int Forward       = 92;       // Move forward
const int Backward      = 163;      // Move backward
const int Stop          = 0;        // Stop

// Current motor state
int currentSpeed = 0;               // Current PWM speed (0-255)
int currentDirection = Stop;        // Current direction
String directionName = "STOPPED";   // Direction name for display

// Function to apply motor speed and direction
void setMotor(int direction, int speed) {
    currentDirection = direction;
    currentSpeed = speed;
    
    // Enable motor driver (LOW = enabled)
    digitalWrite(EN_PIN, LOW);
    
    // Apply PWM speed to both motor channels
    analogWrite(PWM1_PIN, speed);
    analogWrite(PWM2_PIN, speed);
    
    // Send direction to shift register
    digitalWrite(STCP_PIN, LOW);
    shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, direction);
    digitalWrite(STCP_PIN, HIGH);
    
    // Update direction name
    if (direction == Forward) {
        directionName = "FORWARD";
    } else if (direction == Backward) {
        directionName = "BACKWARD";
    } else {
        directionName = "STOPPED";
    }
    
    // Debug output
    Serial.print("[MOTOR] Direction: ");
    Serial.print(directionName);
    Serial.print(" | Speed: ");
    Serial.print(speed);
    Serial.print(" (");
    Serial.print((speed * 100) / 255);
    Serial.println("%)");
}

// Function to parse and execute speed command
void handleSpeedCommand(String value) {
    int speed = value.toInt();
    
    // Validate speed range
    if (speed < 0) speed = 0;
    if (speed > 255) speed = 255;
    
    Serial.print("[CMD] Speed command received: ");
    Serial.println(speed);
    
    // Apply speed with current direction
    setMotor(currentDirection, speed);
}

// Function to parse and execute direction command
void handleDirectionCommand(String value) {
    value.trim();
    
    Serial.print("[CMD] Direction command received: ");
    Serial.println(value);
    
    if (value == "F") {
        // Forward - use current speed or default to medium
        int speed = (currentSpeed > 0) ? currentSpeed : 150;
        setMotor(Forward, speed);
    }
    else if (value == "B") {
        // Backward - use current speed or default to medium
        int speed = (currentSpeed > 0) ? currentSpeed : 150;
        setMotor(Backward, speed);
    }
    else if (value == "S") {
        // Stop - set speed to 0
        setMotor(Stop, 0);
    }
    else {
        Serial.print("[ERROR] Unknown direction: ");
        Serial.println(value);
    }
}

// Function to process incoming serial commands
void processSerialCommand() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            // Check for SPEED command
            if (command.startsWith("SPEED:")) {
                String speedValue = command.substring(6);
                handleSpeedCommand(speedValue);
            }
            // Check for DIR command
            else if (command.startsWith("DIR:")) {
                String dirValue = command.substring(4);
                handleDirectionCommand(dirValue);
            }
            else {
                Serial.print("[ERROR] Unknown command: ");
                Serial.println(command);
            }
        }
    }
}

void setup() {
    // Initialize serial communication (9600 baud to match ESP32)
    Serial.begin(9600);
    delay(100);
    
    // Configure all pins as outputs
    pinMode(SHCP_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(STCP_PIN, OUTPUT);
    pinMode(PWM1_PIN, OUTPUT);
    pinMode(PWM2_PIN, OUTPUT);
    
    // Initialize motor to stopped state
    setMotor(Stop, 0);
    
    // Print startup message
    Serial.println("\n========================================");
    Serial.println("WEB-BASED MOTOR CONTROL - ARDUINO");
    Serial.println("========================================");
    Serial.println("Listening for commands from ESP32-CAM...");
    Serial.println("Command format:");
    Serial.println("  SPEED:XXX  (XXX = 0-255)");
    Serial.println("  DIR:F      (Forward)");
    Serial.println("  DIR:B      (Backward)");
    Serial.println("  DIR:S      (Stop)");
    Serial.println("========================================\n");
    Serial.println("[READY] Waiting for commands...\n");
}

void loop() {
    // Continuously check for and process serial commands
    processSerialCommand();
    
    // Small delay to prevent overwhelming the serial buffer
    delay(10);
}
