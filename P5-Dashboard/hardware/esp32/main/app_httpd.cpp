/*
 * P5-Dashboard — ESP32-CAM HTTP API Server
 * @Description: CORS-enabled REST endpoints for the Express backend.
 *   No embedded HTML — the web UI lives in the React frontend.
 *
 * Endpoints (port 80):
 *   GET  /api/telemetry   → { distance, motorState, connected, uptimeMs }
 *   GET  /api/status      → { ssid, ip, freeHeap, uptimeMs }
 *   GET  /command?cmd=X   → accepts motor command (F/B/L/R/S)
 *   POST /api/command     → accepts JSON { "cmd": "F" }
 *
 * Stream (port 81):
 *   GET  /stream          → MJPEG camera stream
 */

#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "Arduino.h"
#include "WiFi.h"

// ─── Shared state from main.ino ─────────────────────────────────────
extern int gpLed;
extern String WiFiAddr;
extern int currentDistance;
extern bool arduinoConnected;
extern unsigned long lastArduinoResponse;
extern char lastMotorCmd;

void sendMotorCommand(char cmd);

// ─── MJPEG stream helpers ───────────────────────────────────────────
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

// ─── CORS helper ────────────────────────────────────────────────────
static void setCorsHeaders(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
}

// ─── OPTIONS preflight handler ──────────────────────────────────────
static esp_err_t options_handler(httpd_req_t *req)
{
    setCorsHeaders(req);
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ─── GET /api/telemetry ─────────────────────────────────────────────
static esp_err_t telemetry_handler(httpd_req_t *req)
{
    char buf[192];
    snprintf(buf, sizeof(buf),
             "{\"distance\":%d,\"motorState\":\"%c\",\"connected\":%s,\"uptimeMs\":%lu}",
             currentDistance,
             lastMotorCmd,
             arduinoConnected ? "true" : "false",
             millis());

    setCorsHeaders(req);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buf, strlen(buf));
}

// ─── GET /api/status ────────────────────────────────────────────────
static esp_err_t status_handler(httpd_req_t *req)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"ssid\":\"%s\",\"ip\":\"%s\",\"freeHeap\":%u,\"uptimeMs\":%lu}",
             WiFi.SSID().c_str(),
             WiFiAddr.c_str(),
             ESP.getFreeHeap(),
             millis());

    setCorsHeaders(req);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buf, strlen(buf));
}

// ─── GET /command?cmd=X  (P4-compatible) ────────────────────────────
static esp_err_t command_get_handler(httpd_req_t *req)
{
    char query[32] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        char cmd[4] = {0};
        if (httpd_query_key_value(query, "cmd", cmd, sizeof(cmd)) == ESP_OK)
        {
            sendMotorCommand(cmd[0]);
        }
    }

    setCorsHeaders(req);
    httpd_resp_set_type(req, "application/json");
    const char *ok = "{\"status\":\"ok\"}";
    return httpd_resp_send(req, ok, strlen(ok));
}

// ─── POST /api/command  { "cmd": "F" } ─────────────────────────────
static esp_err_t command_post_handler(httpd_req_t *req)
{
    char body[64] = {0};
    int received = httpd_req_recv(req, body, sizeof(body) - 1);
    if (received <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }

    // Minimal JSON parse — look for "cmd":"X"
    char *p = strstr(body, "\"cmd\"");
    if (p)
    {
        p = strchr(p + 5, ':');
        if (p)
        {
            // skip whitespace and quotes
            while (*p && (*p == ':' || *p == ' ' || *p == '"'))
                p++;
            if (*p)
                sendMotorCommand(*p);
        }
    }

    setCorsHeaders(req);
    httpd_resp_set_type(req, "application/json");
    const char *ok = "{\"status\":\"ok\"}";
    return httpd_resp_send(req, ok, strlen(ok));
}

// ─── MJPEG Stream handler (port 81) ────────────────────────────────
static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char part_buf[64];

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
        return res;

    while (true)
    {
        fb = esp_camera_fb_get();
        if (!fb)
        {
            res = ESP_FAIL;
        }
        else
        {
            if (fb->format != PIXFORMAT_JPEG)
            {
                bool ok = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if (!ok)
                    res = ESP_FAIL;
            }
            else
            {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }

        if (res == ESP_OK)
        {
            size_t hlen = snprintf(part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));

        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }

        if (res != ESP_OK)
            break;
    }

    return res;
}

// ─── Server startup ─────────────────────────────────────────────────
void startCameraServer()
{
    // ── Port 80: REST API ───────────────────────────────────────────
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;

    if (httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        // Telemetry
        httpd_uri_t telemetry_uri = {
            .uri = "/api/telemetry", .method = HTTP_GET, .handler = telemetry_handler, .user_ctx = NULL};
        httpd_register_uri_handler(camera_httpd, &telemetry_uri);

        // System status
        httpd_uri_t status_uri = {
            .uri = "/api/status", .method = HTTP_GET, .handler = status_handler, .user_ctx = NULL};
        httpd_register_uri_handler(camera_httpd, &status_uri);

        // Command (GET — backwards compatible with P4)
        httpd_uri_t cmd_get_uri = {
            .uri = "/command", .method = HTTP_GET, .handler = command_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(camera_httpd, &cmd_get_uri);

        // Command (POST — preferred by dashboard backend)
        httpd_uri_t cmd_post_uri = {
            .uri = "/api/command", .method = HTTP_POST, .handler = command_post_handler, .user_ctx = NULL};
        httpd_register_uri_handler(camera_httpd, &cmd_post_uri);

        // OPTIONS preflight for POST
        httpd_uri_t options_uri = {
            .uri = "/api/command", .method = HTTP_OPTIONS, .handler = options_handler, .user_ctx = NULL};
        httpd_register_uri_handler(camera_httpd, &options_uri);
    }

    // ── Port 81: MJPEG stream ───────────────────────────────────────
    httpd_config_t stream_config = HTTPD_DEFAULT_CONFIG();
    stream_config.server_port = 81;
    stream_config.ctrl_port = 32769;

    if (httpd_start(&stream_httpd, &stream_config) == ESP_OK)
    {
        httpd_uri_t stream_uri = {
            .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL};
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}
