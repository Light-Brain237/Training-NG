/*
 * @Date: 2025-12-1
 * ESP32-CAM Distance Display Web Server
 * 
 * Receives distance data from Arduino via Serial
 * Displays on web interface
 */

#include <WiFi.h>

// WiFi credentials
const char* ssid = "TP-LINK_4E81";
const char* password = "Jk123456789";

extern String WiFiAddr = "";

// Function declarations
void startWebServer();
void processSerialData();

void setup() {
  Serial.begin(9600);  // Match Arduino baud rate
  Serial.println();

  // Connect to WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startWebServer();

  Serial.print("Distance Display Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  WiFiAddr = WiFi.localIP().toString();
  Serial.println("' to connect");
}

void loop() {
  // Process incoming Serial data from Arduino
  processSerialData();
  
  delay(10);
}
