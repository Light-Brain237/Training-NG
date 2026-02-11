#include "esp_http_server.h"
#include "esp_timer.h"
#include <Arduino.h>
#include "motor_control.h"

// Current motor state
int currentSpeed = 0;        // 0-255 PWM value
String currentDirection = "STOPPED";

// Function to send command to Arduino via Serial
void sendToArduino(String command) {
    Serial.println(command);
    Serial.flush();
}

// Handler for speed control endpoint
static esp_err_t speed_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    if (remaining > sizeof(buf) - 1) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Parse speed value (0-100 from slider)
    int speedPercent = atoi(buf);
    
    // Convert percentage to PWM (0-255)
    currentSpeed = (speedPercent * 255) / 100;
    
    // Send to Arduino
    String command = "SPEED:" + String(currentSpeed);
    sendToArduino(command);
    
    // Response
    char response[50];
    sprintf(response, "{\"speed\":%d,\"pwm\":%d}", speedPercent, currentSpeed);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    
    return ESP_OK;
}

// Handler for direction control endpoint
static esp_err_t direction_handler(httpd_req_t *req) {
    char buf[10];
    int ret, remaining = req->content_len;

    if (remaining > sizeof(buf) - 1) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Parse direction (F, B, or S)
    String dir = String(buf);
    dir.trim();
    
    // Update current direction
    if (dir == "F") {
        currentDirection = "FORWARD";
    } else if (dir == "B") {
        currentDirection = "BACKWARD";
    } else if (dir == "S") {
        currentDirection = "STOPPED";
    }
    
    // Send to Arduino
    String command = "DIR:" + dir;
    sendToArduino(command);
    
    // Response
    char response[50];
    sprintf(response, "{\"direction\":\"%s\"}", currentDirection.c_str());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    
    return ESP_OK;
}

// Handler for status endpoint
static esp_err_t status_handler(httpd_req_t *req) {
    char response[100];
    int speedPercent = (currentSpeed * 100) / 255;
    sprintf(response, "{\"speed\":%d,\"pwm\":%d,\"direction\":\"%s\"}", 
            speedPercent, currentSpeed, currentDirection.c_str());
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Handler for main page
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, motor_control_html, strlen(motor_control_html));
    return ESP_OK;
}

// Start the web server
void startWebServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    
    httpd_handle_t server = NULL;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register URI handlers
        httpd_uri_t index_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &index_uri);
        
        httpd_uri_t speed_uri = {
            .uri       = "/speed",
            .method    = HTTP_POST,
            .handler   = speed_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &speed_uri);
        
        httpd_uri_t direction_uri = {
            .uri       = "/direction",
            .method    = HTTP_POST,
            .handler   = direction_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &direction_uri);
        
        httpd_uri_t status_uri = {
            .uri       = "/status",
            .method    = HTTP_GET,
            .handler   = status_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &status_uri);
    }
}
