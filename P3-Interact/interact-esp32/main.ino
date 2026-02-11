#include <WiFi.h>

// WiFi credentials
const char* ssid = "TP-LINK_4E81";
const char* password = "Jk123456789";

// Global variables
String WiFiAddr = "";

// External function to start the web server
void startWebServer();

void setup() {
    // Initialize Serial for communication with Arduino
    Serial.begin(9600);
    delay(1000);
    
    Serial.println("\n\n========================================");
    Serial.println("ESP32 MOTOR CONTROL WEB SERVER");
    Serial.println("========================================");
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[SUCCESS] WiFi connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        WiFiAddr = WiFi.localIP().toString();
        
        // Start the web server
        Serial.println("\nStarting web server...");
        startWebServer();
        Serial.println("[SUCCESS] Web server started!");
        
        Serial.println("\n========================================");
        Serial.println("READY!");
        Serial.print("Open browser: http://");
        Serial.println(WiFi.localIP());
        Serial.println("========================================\n");
    } else {
        Serial.println("\n[ERROR] WiFi connection failed!");
        Serial.println("Please check credentials and try again.");
    }
}

void loop() {
    // Main loop - web server runs in background
    delay(10);
}
