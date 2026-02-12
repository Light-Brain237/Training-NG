/*
 * P5-Dashboard — ESP32-CAM Telemetry Firmware
 * @Date: 2025-01-27
 * @Description: Adapted from P4-Interrupt. Exposes REST API endpoints
 *   for an external dashboard (Express + React) instead of serving
 *   the web interface from the ESP32 itself.
 * @Architecture: FreeRTOS, Master-Slave Serial, CORS-enabled HTTP API
 */

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include <WiFi.h>

// ─── WiFi credentials ───────────────────────────────────────────────
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

// ─── Camera pin definitions (AI Thinker ESP32-CAM) ──────────────────
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// ─── Shared state (extern'd by app_httpd.cpp) ───────────────────────
extern int gpLed = 4;
extern String WiFiAddr = "";
extern int currentDistance = 0;
extern bool arduinoConnected = false;
extern unsigned long lastArduinoResponse = 0;
extern char lastMotorCmd = 'S'; // S = Stop

// ─── FreeRTOS primitives ────────────────────────────────────────────
QueueHandle_t distanceQueue;
SemaphoreHandle_t serialMutex;

// Forward declaration — implemented in app_httpd.cpp
void startCameraServer();

// ─── Serial Task (Core 0) — Master-Slave distance polling ──────────
void serialTask(void *parameter)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500); // poll every 500 ms

    while (1)
    {
        if (xSemaphoreTake(serialMutex, portMAX_DELAY))
        {
            Serial.println("GET_DISTANCE");

            unsigned long t0 = millis();
            String response = "";

            while (millis() - t0 < 100) // 100 ms timeout
            {
                if (Serial.available() > 0)
                {
                    response = Serial.readStringUntil('\n');
                    break;
                }
                delay(5);
            }

            if (response.startsWith("DISTANCE:"))
            {
                int dist = response.substring(9).toInt();
                if (dist >= 0 && dist <= 400)
                {
                    xQueueSend(distanceQueue, &dist, 0);
                    currentDistance = dist;
                    arduinoConnected = true;
                    lastArduinoResponse = millis();
                }
            }
            else
            {
                if (millis() - lastArduinoResponse > 2000)
                    arduinoConnected = false;
            }

            xSemaphoreGive(serialMutex);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ─── Motor command helper ───────────────────────────────────────────
void sendMotorCommand(char cmd)
{
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)))
    {
        Serial.println(cmd);
        lastMotorCmd = cmd;

        unsigned long t0 = millis();
        while (millis() - t0 < 50)
        {
            if (Serial.available() > 0)
            {
                String ack = Serial.readStringUntil('\n');
                if (ack.startsWith("ACK:"))
                    break;
            }
            delay(5);
        }

        xSemaphoreGive(serialMutex);
    }
}

// ─── Setup ──────────────────────────────────────────────────────────
void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout

    Serial.begin(9600);
    Serial.setDebugOutput(false);

    serialMutex = xSemaphoreCreateMutex();
    distanceQueue = xQueueCreate(5, sizeof(int));

    pinMode(gpLed, OUTPUT);
    digitalWrite(gpLed, LOW);

    // ── Camera configuration ────────────────────────────────────────
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

    if (psramFound())
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

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed: 0x%x\n", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, FRAMESIZE_CIF);

    // ── WiFi ────────────────────────────────────────────────────────
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    startCameraServer();

    Serial.print("API Ready! http://");
    Serial.println(WiFi.localIP());
    WiFiAddr = WiFi.localIP().toString();

    delay(2000);
    Serial.println("GET_DISTANCE"); // initial handshake

    // ── Serial task on Core 0 ───────────────────────────────────────
    xTaskCreatePinnedToCore(
        serialTask, "SerialTask", 4096, NULL,
        3, // high priority
        NULL,
        0 // Core 0
    );
}

void loop()
{
    delay(10); // Core 1 handles camera & HTTP
}
