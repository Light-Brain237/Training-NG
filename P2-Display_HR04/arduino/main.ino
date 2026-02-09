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
char serialData;            // Store serial port received data
char cmd;                   // Store Bluetooth received command


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


// Serial data reception function


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

    
    Serial.println("\n========================================");
    Serial.println("Robot Car - Distance Display Mode");
    Serial.println("========================================");
    Serial.println("Sending distance data to ESP32...");
    Serial.println("========================================\n");
} 

void loop()
{
    distance = SR04(Trig, Echo);  // Ultrasonic distance measurement
    
    // Send distance to ESP32 in format: DISTANCE:XX.X
    Serial.print("DISTANCE:");
    Serial.println(distance);
    delay(500);  // Wait before next measurement    
}
