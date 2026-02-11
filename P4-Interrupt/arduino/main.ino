// Date: 2024-12-02
// Unified Robot Control System - Arduino Controller
// Master-Slave Protocol Implementation

// PWM control pins
#define PWM1_PIN            5
#define PWM2_PIN            6

// 74HCT595N chip pins
#define SHCP_PIN            2       // Shift register clock pin
#define EN_PIN              7       // Enable pin
#define DATA_PIN            8       // Serial data pin
#define STCP_PIN            4       // Storage register clock pin

// Ultrasonic sensor control pins
const int Trig = 12;
const int Echo = 13;

// Define car movement control values
const int Forward       = 92;       // Move forward
const int Backward      = 163;      // Move backward
const int Left          = 149;      // Move left
const int Right         = 106;      // Move right
const int Top_Left      = 20;       // Move top-left (diagonal)
const int Bottom_Left   = 129;      // Move bottom-left (diagonal)
const int Top_Right     = 72;       // Move top-right (diagonal)
const int Bottom_Right  = 34;       // Move bottom-right (diagonal)
const int Turn_Left     = 172;      // Turn left
const int Turn_Right    = 83;       // Turn right
const int Stop          = 0;        // Stop

int distance = 0;           // Variable to store ultrasonic sensor measurement
char currentCmd = 'S';      // Current command being executed
unsigned long lastDistanceRead = 0;
const unsigned long DISTANCE_READ_INTERVAL = 100;  // Read every 100ms

// Motor driver function
void Motor(int Dir, int Speed1)    
{
    digitalWrite(EN_PIN, LOW);
    analogWrite(PWM1_PIN, Speed1);
    analogWrite(PWM2_PIN, Speed1);

    digitalWrite(STCP_PIN, LOW);
    shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, Dir);
    digitalWrite(STCP_PIN, HIGH);
}

// Ultrasonic distance measurement function
int SR04(int Trig, int Echo)
{
    float cm = 0;

    digitalWrite(Trig, LOW);
    delayMicroseconds(2); 
    digitalWrite(Trig, HIGH);
    delayMicroseconds(15); 
    digitalWrite(Trig, LOW);

    cm = pulseIn(Echo, HIGH) / 58.8; 
    delay(10);
    return cm; 
}

// Obstacle avoidance function
void AvoidingObstacles()
{
    if(distance > 20)  // If distance > 20cm
    {
        Motor(Forward, 250);  // Move forward
    }
    else  // Otherwise move backward then turn left
    {
        Motor(Backward, 250);  // Move backward
        delay(500);
        Motor(Turn_Left, 250);  // Turn left
        delay(500);
        currentCmd = 'S';  // Stop after avoidance
    }
}

// Execute motor command based on received character
void ExecuteCommand(char cmd)
{
    currentCmd = cmd;
    
    switch(cmd)
    {
        case 'F':  // Forward with obstacle avoidance
            AvoidingObstacles();
            break;
            
        case 'B':  // Backward
            Motor(Backward, 250);
            break;
            
        case 'L':  // Move left
            Motor(Left, 250);
            break;
            
        case 'R':  // Move right
            Motor(Right, 250);
            break;
            
        case 'A':  // Turn left
            Motor(Turn_Left, 250);
            break;
            
        case 'D':  // Turn right
            Motor(Turn_Right, 250);
            break;
            
        case 'Q':  // Top-left diagonal
            Motor(Top_Left, 250);
            break;
            
        case 'W':  // Top-right diagonal
            Motor(Top_Right, 250);
            break;
            
        case 'Z':  // Bottom-left diagonal
            Motor(Bottom_Left, 250);
            break;
            
        case 'X':  // Bottom-right diagonal
            Motor(Bottom_Right, 250);
            break;
            
        case 'S':  // Stop
            Motor(Stop, 0);
            break;
            
        default:
            // Unknown command, do nothing
            break;
    }
}

// Process incoming serial commands (Master-Slave Protocol)
void ProcessSerialCommands()
{
    if(Serial.available() > 0)
    {
        String incoming = Serial.readStringUntil('\n');
        incoming.trim();
        
        // Check if it's a distance request from ESP32 (Master)
        if(incoming == "GET_DISTANCE")
        {
            // Respond with distance data
            Serial.print("DISTANCE:");
            Serial.println(distance);
        }
        // Check if it's a motor command (single character)
        else if(incoming.length() == 1)
        {
            char cmd = incoming.charAt(0);
            ExecuteCommand(cmd);
            // Send acknowledgment
            Serial.print("ACK:");
            Serial.println(cmd);
        }
    }
}

void setup() 
{
    Serial.begin(9600);  // Set serial baud rate to 9600

    // Configure as output mode
    pinMode(SHCP_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(STCP_PIN, OUTPUT);
    pinMode(PWM1_PIN, OUTPUT);
    pinMode(PWM2_PIN, OUTPUT);

    pinMode(Trig, OUTPUT);  // Set Trig as output
    pinMode(Echo, INPUT);   // Set Echo as input

    Motor(Stop, 0);  // Initialize with car stopped
    
    delay(100);  // Short delay for serial to stabilize
    Serial.println("ARDUINO_READY");
} 

void loop()
{
    // Read distance periodically
    unsigned long currentMillis = millis();
    if(currentMillis - lastDistanceRead >= DISTANCE_READ_INTERVAL)
    {
        distance = SR04(Trig, Echo);
        lastDistanceRead = currentMillis;
        
        // If currently moving forward, check for obstacles
        if(currentCmd == 'F')
        {
            AvoidingObstacles();
        }
    }
    
    // Process any incoming serial commands
    ProcessSerialCommands();
    
    delay(10);  // Small delay to prevent overwhelming the serial buffer
}
