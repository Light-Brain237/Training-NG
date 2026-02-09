/*
 * HTTP Server and Request Handlers
 * 
 * This file contains all HTTP endpoint handlers for the distance display system.
 */

#include <Arduino.h>
#include <esp_http_server.h>
#include "distance_display.h"

// Global variables
httpd_handle_t server = NULL;
float currentDistance = 0.0;
bool dataValid = false;
unsigned long lastDataTime = 0;
const unsigned long DATA_TIMEOUT = 3000; // 3 seconds

// Function to process incoming Serial data from Arduino
void processSerialData() {
  static String buffer = "";
  
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n') {
      // Complete command received
      if (buffer.startsWith("DISTANCE:")) {
        String distanceStr = buffer.substring(9);
        distanceStr.trim();
        
        if (distanceStr == "ERROR") {
          dataValid = false;
          Serial.println("[ESP32] Received: Out of range");
        } else {
          currentDistance = distanceStr.toFloat();
          dataValid = true;
          lastDataTime = millis();
          Serial.print("[ESP32] Distance: ");
          Serial.print(currentDistance, 1);
          Serial.println(" cm");
        }
      }
      buffer = "";
    } else {
      buffer += c;
    }
    
    // Prevent buffer overflow
    if (buffer.length() > 50) {
      buffer = "";
    }
  }
  
  // Check for data timeout
  if (dataValid && (millis() - lastDataTime > DATA_TIMEOUT)) {
    dataValid = false;
    Serial.println("[ESP32] Data timeout - no recent updates from Arduino");
  }
}

// Handler for root page (GET /)
static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, distance_display_html, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

// Handler for distance data (GET /distance)
static esp_err_t distance_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  
  char json[100];
  
  if (dataValid) {
    snprintf(json, sizeof(json), 
             "{\"distance\":%.1f,\"unit\":\"cm\",\"valid\":true}", 
             currentDistance);
  } else {
    snprintf(json, sizeof(json), 
             "{\"distance\":0,\"unit\":\"cm\",\"valid\":false,\"error\":\"No data\"}");
  }
  
  httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

// Handler for status (GET /status)
static esp_err_t status_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  
  char json[150];
  unsigned long uptime = millis() / 1000;
  
  snprintf(json, sizeof(json), 
           "{\"distance\":%.1f,\"valid\":%s,\"uptime\":%lu,\"lastUpdate\":%lu}", 
           currentDistance,
           dataValid ? "true" : "false",
           uptime,
           dataValid ? ((millis() - lastDataTime) / 1000) : 0);
  
  httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

// Start the web server
void startWebServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32768;
  config.max_open_sockets = 7;
  config.lru_purge_enable = true;
  
  // URI handlers
  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };
  
  httpd_uri_t distance_uri = {
    .uri       = "/distance",
    .method    = HTTP_GET,
    .handler   = distance_handler,
    .user_ctx  = NULL
  };
  
  httpd_uri_t status_uri = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_handler,
    .user_ctx  = NULL
  };
  
  // Start server
  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_register_uri_handler(server, &index_uri);
    httpd_register_uri_handler(server, &distance_uri);
    httpd_register_uri_handler(server, &status_uri);
  }
}
