/*
 * @Date: 2024-12-02
 * @Description: Unified ESP32-CAM Robot Control System
 * @Features: Camera Stream + Distance Display + Motor Control
 * @Architecture: FreeRTOS with Master-Slave Serial Protocol
 */

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include <WiFi.h>

// Select camera model
#define CAMERA_MODEL_AI_THINKER

const char* ssid = "TP-LINK_4E81";
const char* password = "Jk123456789";

// Camera pin definitions for AI Thinker
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

extern int gpLed = 4;
extern String WiFiAddr = "";
extern int currentDistance = 0;
extern bool arduinoConnected = false;
extern unsigned long lastArduinoResponse = 0;

// FreeRTOS queue for distance data
QueueHandle_t distanceQueue;

// Mutex for Serial access
SemaphoreHandle_t serialMutex;

void startCameraServer();

// Task to request distance from Arduino (Master-Slave Protocol)
void serialTask(void *parameter)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500);  // Request every 500ms
    
    while(1)
    {
        // Take mutex before accessing Serial
        if(xSemaphoreTake(serialMutex, portMAX_DELAY))
        {
            // Request distance from Arduino
            Serial.println("GET_DISTANCE");
            
            // Wait for response (with timeout)
            unsigned long startTime = millis();
            String response = "";
            
            while(millis() - startTime < 100)  // 100ms timeout
            {
                if(Serial.available() > 0)
                {
                    response = Serial.readStringUntil('\n');
                    break;
                }
                delay(5);
            }
            
            // Parse distance response
            if(response.startsWith("DISTANCE:"))
            {
                int dist = response.substring(9).toInt();
                if(dist >= 0 && dist <= 400)
                {
                    // Send distance to queue
                    xQueueSend(distanceQueue, &dist, 0);
                    currentDistance = dist;
                    
                    // Mark Arduino as connected
                    arduinoConnected = true;
                    lastArduinoResponse = millis();
                }
            }
            else
            {
                // No valid response - check timeout
                if(millis() - lastArduinoResponse > 2000)
                {
                    arduinoConnected = false;
                }
            }
            
            xSemaphoreGive(serialMutex);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Function to send motor commands to Arduino
void sendMotorCommand(char cmd)
{
    // Take mutex before accessing Serial
    if(xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)))
    {
        Serial.println(cmd);
        
        // Wait for ACK (with short timeout)
        unsigned long startTime = millis();
        while(millis() - startTime < 50)
        {
            if(Serial.available() > 0)
            {
                String ack = Serial.readStringUntil('\n');
                if(ack.startsWith("ACK:"))
                {
                    break;
                }
            }
            delay(5);
        }
        
        xSemaphoreGive(serialMutex);
    }
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disable brownout detector
    
    Serial.begin(9600);
    Serial.setDebugOutput(false);
    
    // Create mutex for Serial access
    serialMutex = xSemaphoreCreateMutex();
    
    // Create queue for distance data
    distanceQueue = xQueueCreate(5, sizeof(int));
    
    pinMode(gpLed, OUTPUT);
    digitalWrite(gpLed, LOW);
    
    // Camera configuration
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    if(psramFound())
    {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }
    
    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if(err != ESP_OK)
    {
        Serial.printf("Camera init failed: 0x%x\n", err);
        return;
    }
    
    // Set frame size
    sensor_t * s = esp_camera_sensor_get();
    s->set_framesize(s, FRAMESIZE_CIF);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    
    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    
    // Start camera web server
    startCameraServer();
    
    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
    WiFiAddr = WiFi.localIP().toString();
    
    // Wait for Arduino to be ready
    delay(2000);
    Serial.println("GET_DISTANCE");  // Initial handshake
    
    // Create FreeRTOS task for serial communication
    xTaskCreatePinnedToCore(
        serialTask,           // Task function
        "SerialTask",         // Task name
        4096,                 // Stack size
        NULL,                 // Parameters
        3,                    // Priority (high)
        NULL,                 // Task handle
        0                     // Core 0
    );
}

void loop()
{
    // Main loop runs on Core 1 (handles camera and HTTP)
    delay(10);
}
