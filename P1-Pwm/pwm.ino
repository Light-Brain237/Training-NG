// ============================================
// PWM SPEED CONTROL PROGRAM
// ============================================
// Commands:
//   'L' = Low speed (15%)
//   'M' = Medium speed (65%)
//   'H' = High speed (100%)
// ============================================

// PWM control pins
#define PWM1_PIN            5
#define PWM2_PIN            6

// 74HCT595N shift register pins
#define SHCP_PIN            2       // Shift register clock pin
#define EN_PIN              7       // Enable pin
#define DATA_PIN            8       // Serial data pin
#define STCP_PIN            4       // Storage register clock pin

// Motor direction values (same as main.cpp)
const int Forward       = 92;       // Move forward
const int Backward      = 163;      // Move backward
const int Stop          = 0;        // Stop

// PWM Speed Levels (0-255 scale)
const int SPEED_LOW     = 98;       // 15% of 255 = ~38
const int SPEED_MEDIUM  = 166;      // 65% of 255 = ~166
const int SPEED_HIGH    = 255;      // 100% of 255 = 255

// Current speed setting
int currentSpeed = SPEED_MEDIUM;    // Default to medium speed
char speedMode = 'M';               // Current speed mode
int currentDirection = Stop;        // Track current motor direction

// Function to set motor speed and direction
void setMotorSpeed(int direction, int speed) {
    // Save the current direction
    currentDirection = direction;
    
    // Debug output
    Serial.println("--- setMotorSpeed called ---");
    Serial.print("Direction value: ");
    Serial.println(direction);
    Serial.print("Speed (PWM): ");
    Serial.println(speed);
    
    // Enable the motor driver
    digitalWrite(EN_PIN, LOW);
    Serial.println("EN_PIN set LOW (enabled)");
    
    // Set PWM speed on both motor pins
    analogWrite(PWM1_PIN, speed);
    analogWrite(PWM2_PIN, speed);
    Serial.print("PWM applied to pins ");
    Serial.print(PWM1_PIN);
    Serial.print(" and ");
    Serial.println(PWM2_PIN);
    
    // Set direction via shift register
    digitalWrite(STCP_PIN, LOW);
    shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, direction);
    digitalWrite(STCP_PIN, HIGH);
    Serial.println("Direction sent to shift register");
    Serial.println("--- Motor command complete ---\n");
}

// Function to change speed level
void changeSpeedLevel(char level) {
    switch(level) {
        case 'L':
        case 'l':
            currentSpeed = SPEED_LOW;
            speedMode = 'L';
            Serial.println("\n>>> Speed set to LOW (15%)");
            Serial.print(">>> PWM Value: ");
            Serial.println(currentSpeed);
            break;
            
        case 'M':
        case 'm':
            currentSpeed = SPEED_MEDIUM;
            speedMode = 'M';
            Serial.println("\n>>> Speed set to MEDIUM (65%)");
            Serial.print(">>> PWM Value: ");
            Serial.println(currentSpeed);
            break;
            
        case 'H':
        case 'h':
            currentSpeed = SPEED_HIGH;
            speedMode = 'H';
            Serial.println("\n>>> Speed set to HIGH (100%)");
            Serial.print(">>> PWM Value: ");
            Serial.println(currentSpeed);
            break;
            
        default:
            Serial.print("Unknown speed command: ");
            Serial.println(level);
            return;
    }
    
    // If motors are currently running, apply the new speed immediately
    if (currentDirection != Stop) {
        setMotorSpeed(currentDirection, currentSpeed);
        Serial.println(">>> Speed applied to running motors");
    }
}

// Function to process movement commands with current speed
void processCommand(char cmd) {
    switch(cmd) {
        case 'F':
        case 'f':
            setMotorSpeed(Forward, currentSpeed);
            Serial.print("Moving FORWARD at ");
            Serial.print(speedMode);
            Serial.print(" speed (");
            Serial.print(currentSpeed);
            Serial.println(")");
            break;
            
        case 'B':
        case 'b':
            setMotorSpeed(Backward, currentSpeed);
            Serial.print("Moving BACKWARD at ");
            Serial.print(speedMode);
            Serial.print(" speed (");
            Serial.print(currentSpeed);
            Serial.println(")");
            break;
            
        case 'S':
        case 's':
            setMotorSpeed(Stop, 0);
            Serial.println("STOPPED");
            break;
            
        default:
            // Ignore unknown commands
            break;
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    delay(100);
    
    Serial.println("\n\n=== PWM TEST STARTING ===");
    Serial.println("Configuring pins...");
    
    // Configure pins as outputs
    pinMode(SHCP_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(STCP_PIN, OUTPUT);
    pinMode(PWM1_PIN, OUTPUT);
    pinMode(PWM2_PIN, OUTPUT);
    
    Serial.println("Pins configured:");
    Serial.print("  SHCP_PIN: ");
    Serial.println(SHCP_PIN);
    Serial.print("  EN_PIN: ");
    Serial.println(EN_PIN);
    Serial.print("  DATA_PIN: ");
    Serial.println(DATA_PIN);
    Serial.print("  STCP_PIN: ");
    Serial.println(STCP_PIN);
    Serial.print("  PWM1_PIN: ");
    Serial.println(PWM1_PIN);
    Serial.print("  PWM2_PIN: ");
    Serial.println(PWM2_PIN);
    
    // Initialize motor to stopped state
    Serial.println("\nInitializing motor to STOP...");
    setMotorSpeed(Stop, 0);
    
    // Print welcome message
    Serial.println("\n========================================");
    Serial.println("PWM SPEED CONTROL TEST PROGRAM");
    Serial.println("========================================");
    Serial.println("Speed Commands:");
    Serial.println("  L = Low speed (15% / PWM 38)");
    Serial.println("  M = Medium speed (65% / PWM 166)");
    Serial.println("  H = High speed (100% / PWM 255)");
    Serial.println("\nMovement Commands:");
    Serial.println("  F = Forward");
    Serial.println("  B = Backward");
    Serial.println("  S = Stop");
    Serial.println("========================================");
    Serial.print("\nCurrent Speed: ");
    Serial.print(speedMode);
    Serial.print(" (PWM: ");
    Serial.print(currentSpeed);
    Serial.println(")");
    Serial.println("\nReady! Waiting for commands...\n");
}

void loop() {
    // Display prompt and wait for user input
    Serial.println("\n>>> Enter your command (L/M/H for speed, F/B/S for movement): ");
    
    // Wait for user to enter a command
    while (Serial.available() == 0) {
        // Wait for input
    }
    
    // Read the command
    char incomingChar = Serial.read();
    
    // Clear any remaining characters in the buffer (like newline)
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    // Echo the command
    Serial.print("Command received: ");
    Serial.println(incomingChar);
    
    // Process speed change commands
    if (incomingChar == 'L' || incomingChar == 'l') {
        changeSpeedLevel(incomingChar);
    }
    else if (incomingChar == 'M' || incomingChar == 'm') {
        changeSpeedLevel(incomingChar);
    }
    else if (incomingChar == 'H' || incomingChar == 'h') {
        changeSpeedLevel(incomingChar);
    }
    // Process movement commands
    else if (incomingChar == 'F' || incomingChar == 'f') {
        processCommand(incomingChar);
    }
    else if (incomingChar == 'B' || incomingChar == 'b') {
        processCommand(incomingChar);
    }
    else if (incomingChar == 'S' || incomingChar == 's') {
        processCommand(incomingChar);
    }
    // Unknown command
    else if (incomingChar != '\n' && incomingChar != '\r' && incomingChar != ' ') {
        Serial.print("Unknown command: ");
        Serial.println(incomingChar);
        Serial.println("Valid commands: L, M, H (speed) | F, B, S (movement)");
    }
    
    delay(500);  // Small delay before next prompt
}
