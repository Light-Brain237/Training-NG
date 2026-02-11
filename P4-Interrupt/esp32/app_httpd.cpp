//2024-12-02
// Unified Robot Control System - HTTP Server
// Camera Stream + Distance Display + Motor Control

#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "Arduino.h"

extern int gpLed;
extern String WiFiAddr;
extern int currentDistance;
extern bool arduinoConnected;
extern unsigned long lastArduinoResponse;

// Forward declaration
void sendMotorCommand(char cmd);

typedef struct {
    size_t size;
    size_t index;
    size_t count;
    int sum;
    int * values;
} ra_filter_t;

typedef struct {
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static ra_filter_t ra_filter;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static ra_filter_t * ra_filter_init(ra_filter_t * filter, size_t sample_size)
{
    memset(filter, 0, sizeof(ra_filter_t));
    filter->values = (int *)malloc(sample_size * sizeof(int));
    if(!filter->values) return NULL;
    memset(filter->values, 0, sample_size * sizeof(int));
    filter->size = sample_size;
    return filter;
}

static int ra_filter_run(ra_filter_t * filter, int value)
{
    if(!filter->values) return value;
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index = (filter->index + 1) % filter->size;
    if(filter->count < filter->size) filter->count++;
    return filter->sum / filter->count;
}

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len)
{
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index) j->len = 0;
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) return 0;
    j->len += len;
    return len;
}

static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    
    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) return res;
    
    while(true)
    {
        fb = esp_camera_fb_get();
        if(!fb)
        {
            res = ESP_FAIL;
        }
        else
        {
            if(fb->format != PIXFORMAT_JPEG)
            {
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if(!jpeg_converted) res = ESP_FAIL;
            }
            else
            {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        
        if(res == ESP_OK)
        {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        
        if(fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if(_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        
        if(res != ESP_OK) break;
    }
    
    return res;
}

// Distance API endpoint
static esp_err_t distance_handler(httpd_req_t *req)
{
    char response[64];
    snprintf(response, sizeof(response), "{\"distance\":%d,\"connected\":%s}", 
             currentDistance, arduinoConnected ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, response, strlen(response));
}

// Unified web interface with camera, distance, and joystick controls
static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    
    // Serve optimized embedded HTML with joystick control
    String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=0'>";
    page += "<title>Robot Control</title><style>*{margin:0;padding:0;box-sizing:border-box}";
    page += "body{font-family:'Segoe UI',Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px 10px;display:flex;justify-content:center;align-items:center}";
    page += ".container{max-width:500px;width:100%;background:#fff;border-radius:20px;padding:25px;box-shadow:0 10px 30px rgba(0,0,0,0.3)}";
    page += "h1{color:#333;text-align:center;margin-bottom:20px;font-size:28px;font-weight:600}";
    page += ".camera-box{border-radius:12px;overflow:hidden;box-shadow:0 4px 12px rgba(0,0,0,0.15);background:#000;margin-bottom:20px}";
    page += ".camera-box img{width:100%;display:block}";
    page += ".distance-box{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;padding:20px;border-radius:12px;margin-bottom:20px;box-shadow:0 4px 12px rgba(0,0,0,0.15)}";
    page += ".distance-label{font-size:14px;opacity:0.9;text-transform:uppercase;letter-spacing:1px;text-align:center}";
    page += ".distance-value{font-size:56px;font-weight:bold;text-align:center;margin:15px 0;text-shadow:0 2px 4px rgba(0,0,0,0.2)}";
    page += ".proximity-bar{height:10px;background:rgba(255,255,255,0.25);border-radius:5px;overflow:hidden;margin:15px 0}";
    page += ".proximity-fill{height:100%;transition:width 0.3s ease,background-color 0.3s ease;border-radius:5px}";
    page += ".status-text{text-align:center;font-size:16px;font-weight:500;margin-top:10px}";
    page += ".section-title{font-size:16px;color:#666;text-align:center;margin-bottom:15px;font-weight:600;text-transform:uppercase;letter-spacing:1px}";
    page += ".joystick-container{display:flex;justify-content:center;margin:20px 0;padding:20px}";
    page += ".joystick-wrapper{position:relative;width:200px;height:200px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);border-radius:50%;box-shadow:inset 0 4px 8px rgba(0,0,0,0.2),0 4px 12px rgba(102,126,234,0.3);display:flex;justify-content:center;align-items:center;touch-action:none;user-select:none}";
    page += ".joystick-base{position:absolute;width:180px;height:180px;border-radius:50%;background:radial-gradient(circle at 30% 30%,rgba(255,255,255,0.15),rgba(255,255,255,0.05));box-shadow:inset 0 2px 4px rgba(0,0,0,0.15)}";
    page += ".joystick-stick{position:absolute;width:80px;height:80px;border-radius:50%;background:radial-gradient(circle at 30% 30%,#667eea,#764ba2);box-shadow:0 4px 12px rgba(102,126,234,0.5),inset 0 -2px 4px rgba(0,0,0,0.2),inset 0 2px 4px rgba(255,255,255,0.3);cursor:grab;transition:background 0.2s ease,box-shadow 0.2s ease;z-index:10}";
    page += ".joystick-stick:active{cursor:grabbing;box-shadow:0 2px 8px rgba(0,0,0,0.4),inset 0 -2px 4px rgba(0,0,0,0.3)}";
    page += ".joystick-center{position:absolute;width:15px;height:15px;border-radius:50%;background:rgba(255,255,255,0.3);pointer-events:none}";
    page += ".joystick-direction{position:absolute;font-size:14px;font-weight:bold;color:#fff;opacity:0.5;user-select:none}";
    page += ".dir-up{top:5px;left:50%;transform:translateX(-50%)}.dir-down{bottom:5px;left:50%;transform:translateX(-50%)}.dir-left{left:5px;top:50%;transform:translateY(-50%)}.dir-right{right:5px;top:50%;transform:translateY(-50%)}";
    page += ".current-command{text-align:center;margin:15px 0;font-size:18px;font-weight:bold;color:#764ba2;min-height:25px}";
    page += ".control-row{display:flex;justify-content:center;gap:10px;margin-bottom:10px}";
    page += "button{border:none;border-radius:10px;font-size:14px;font-weight:600;cursor:pointer;transition:all 0.2s ease;user-select:none;box-shadow:0 2px 8px rgba(0,0,0,0.15);padding:10px 20px}";
    page += "button:active{transform:scale(0.95);box-shadow:0 1px 4px rgba(0,0,0,0.2)}";
    page += ".btn-turn{background:linear-gradient(135deg,#ffeaa7 0%,#fdcb6e 100%);color:#2d3436}";
    page += ".btn-stop{background:linear-gradient(135deg,#ff7675 0%,#d63031 100%);color:#fff;font-size:18px;padding:12px 30px}";
    page += ".btn-light{background:linear-gradient(135deg,#00b894 0%,#00cec9 100%);color:#fff}";
    page += ".joy-forward{background:radial-gradient(circle at 30% 30%,#00b894,#00cec9)!important}";
    page += ".joy-backward{background:radial-gradient(circle at 30% 30%,#fdcb6e,#e17055)!important}";
    page += ".joy-left{background:radial-gradient(circle at 30% 30%,#6c5ce7,#a29bfe)!important}";
    page += ".joy-right{background:radial-gradient(circle at 30% 30%,#0984e3,#74b9ff)!important}";
    page += ".joy-topleft{background:radial-gradient(circle at 30% 30%,#00b894,#55efc4)!important}";
    page += ".joy-topright{background:radial-gradient(circle at 30% 30%,#00cec9,#81ecec)!important}";
    page += ".joy-bottomleft{background:radial-gradient(circle at 30% 30%,#fdcb6e,#ffeaa7)!important}";
    page += ".joy-bottomright{background:radial-gradient(circle at 30% 30%,#e17055,#fab1a0)!important}";
    page += ".joy-stop{background:radial-gradient(circle at 30% 30%,#667eea,#764ba2)!important}";
    page += ".status-container{display:flex;align-items:center;justify-content:center;gap:10px;margin-top:15px}";
    page += ".status-indicator{width:12px;height:12px;border-radius:50%;transition:background 0.3s}";
    page += ".status-indicator.connected{background:#28a745;box-shadow:0 0 8px #28a745}";
    page += ".status-indicator.error{background:#dc3545;box-shadow:0 0 8px #dc3545}";
    page += ".status-text{font-size:14px;font-weight:600;color:#333}";
    page += ".error-msg{background:#f8d7da;color:#721c24;padding:10px;border-radius:8px;margin-top:10px;font-size:14px;display:none}";
    page += "</style></head><body><div class='container'><h1>🤖 Robot Control</h1>";
    
    page += "<div class='camera-box'><img src='http://" + WiFiAddr + ":81/stream'></div>";
    
    page += "<div class='distance-box'><div class='distance-label'>Distance Sensor</div><div class='distance-value' id='d'>--</div>";
    page += "<div class='proximity-bar'><div class='proximity-fill' id='p'></div></div><div class='status-text' id='s'>Initializing...</div></div>";
    
    page += "<div class='section-title'>Joystick Control</div><div class='joystick-container'><div class='joystick-wrapper' id='jw'>";
    page += "<div class='joystick-base'></div><div class='joystick-center'></div><div class='joystick-stick' id='js'></div>";
    page += "<span class='joystick-direction dir-up'>↑</span><span class='joystick-direction dir-down'>↓</span>";
    page += "<span class='joystick-direction dir-left'>←</span><span class='joystick-direction dir-right'>→</span></div></div>";
    page += "<div class='current-command' id='c'>Ready</div>";
    
    page += "<div class='control-row'><button class='btn-turn' onmousedown=\"x('A')\" onmouseup=\"x('S')\" ontouchstart=\"x('A')\" ontouchend=\"x('S')\">↺ Turn Left</button>";
    page += "<button class='btn-stop' onclick=\"x('S')\">■ STOP</button>";
    page += "<button class='btn-turn' onmousedown=\"x('D')\" onmouseup=\"x('S')\" ontouchstart=\"x('D')\" ontouchend=\"x('S')\">Turn Right ↻</button></div>";
    
    page += "<div class='control-row'><button class='btn-light' onclick=\"fetch('/ledon')\">💡 Light ON</button>";
    page += "<button class='btn-light' onclick=\"fetch('/ledoff')\">🌙 Light OFF</button></div>";
    page += "<div class='status-container'><div class='status-indicator error' id='si'></div>";
    page += "<span class='status-text' id='st'>Connecting to ESP32...</span></div>";
    page += "<div class='error-msg' id='em'>Failed to connect to ESP32</div></div>";
    
    page += "<script>let a=0,cc='S';function x(c){fetch('/'+c).then(()=>{us(1)}).catch(e=>{us(0);console.error(e)})}";
    page += "function us(ok){let si=document.getElementById('si'),st=document.getElementById('st'),em=document.getElementById('em');";
    page += "if(ok){si.className='status-indicator connected';st.textContent='System Connected';em.style.display='none'}";
    page += "else{si.className='status-indicator error';st.textContent='Arduino Disconnected';em.textContent='No response from Arduino';em.style.display='block'}}";
    page += "function u(){fetch('/distance').then(r=>r.json()).then(v=>{us(v.connected);let t=v.distance;document.getElementById('d').innerText=t+' cm';";
    page += "let b=document.getElementById('p'),m=document.getElementById('s');";
    page += "if(t<10){b.style.width='100%';b.style.background='linear-gradient(90deg,#ff4444 0%,#cc0000 100%)';m.innerText='⚠️ VERY CLOSE'}";
    page += "else if(t<30){b.style.width='75%';b.style.background='linear-gradient(90deg,#ffaa00 0%,#ff8800 100%)';m.innerText='⚠️ Close'}";
    page += "else if(t<100){b.style.width='40%';b.style.background='linear-gradient(90deg,#ffee00 0%,#ffcc00 100%)';m.innerText='✓ Safe Distance'}";
    page += "else{b.style.width='20%';b.style.background='linear-gradient(90deg,#44ff44 0%,#00dd00 100%)';m.innerText='✓ Clear'}";
    page += "}).catch(e=>{us(0);console.error('Error:',e)})}setInterval(u,500);u();";
    
    page += "const w=document.getElementById('jw'),st=document.getElementById('js'),cd=document.getElementById('c');let md=60;";
    page += "function hs(e){a=1;st.style.transition='none';hm(e)}";
    page += "function hm(e){if(!a)return;e.preventDefault();const cx=e.touches?e.touches[0].clientX:e.clientX,cy=e.touches?e.touches[0].clientY:e.clientY;";
    page += "const r=w.getBoundingClientRect();let ox=cx-r.left-100,oy=cy-r.top-100;const ds=Math.sqrt(ox*ox+oy*oy);";
    page += "if(ds>md){const an=Math.atan2(oy,ox);ox=Math.cos(an)*md;oy=Math.sin(an)*md}st.style.transform=`translate(${ox}px,${oy}px)`;";
    page += "const nc=gc(ox,oy);if(nc!==cc){cc=nc;x(nc);uc(nc);uco(nc)}}";
    page += "function he(e){if(!a)return;a=0;st.style.transition='transform 0.2s ease, background 0.2s ease';st.style.transform='translate(0px,0px)';cc='S';x('S');uc('S');uco('S')}";
    page += "function gc(ox,oy){const th=15,ds=Math.sqrt(ox*ox+oy*oy);if(ds<th)return 'S';const an=Math.atan2(oy,ox)*(180/Math.PI);";
    page += "if(an>=-22.5&&an<22.5)return 'R';else if(an>=22.5&&an<67.5)return 'X';else if(an>=67.5&&an<112.5)return 'B';";
    page += "else if(an>=112.5&&an<157.5)return 'Z';else if(an>=157.5||an<-157.5)return 'L';else if(an>=-157.5&&an<-112.5)return 'Q';";
    page += "else if(an>=-112.5&&an<-67.5)return 'F';else if(an>=-67.5&&an<-22.5)return 'W';return 'S'}";
    page += "function uc(k){const n={'F':'↑ Forward','B':'↓ Backward','L':'← Left','R':'→ Right','Q':'↖ Top-Left','W':'↗ Top-Right',";
    page += "'Z':'↙ Bottom-Left','X':'↘ Bottom-Right','A':'↺ Turn Left','D':'↻ Turn Right','S':'■ Stop'};cd.textContent=n[k]||'Ready'}";
    page += "function uco(k){st.className='joystick-stick';const m={'F':'joy-forward','B':'joy-backward','L':'joy-left','R':'joy-right',";
    page += "'Q':'joy-topleft','W':'joy-topright','Z':'joy-bottomleft','X':'joy-bottomright','S':'joy-stop'};if(m[k])st.classList.add(m[k])}";
    page += "st.addEventListener('mousedown',hs);document.addEventListener('mousemove',hm);document.addEventListener('mouseup',he);";
    page += "st.addEventListener('touchstart',hs,{passive:false});document.addEventListener('touchmove',hm,{passive:false});";
    page += "document.addEventListener('touchend',he,{passive:false});</script></body></html>";
    
    return httpd_resp_send(req, &page[0], strlen(&page[0]));
}

// Motor command handlers
static esp_err_t go_handler(httpd_req_t *req) {
    sendMotorCommand('F');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t back_handler(httpd_req_t *req) {
    sendMotorCommand('B');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t stop_handler(httpd_req_t *req) {
    sendMotorCommand('S');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t left_handler(httpd_req_t *req) {
    sendMotorCommand('L');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t right_handler(httpd_req_t *req) {
    sendMotorCommand('R');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t turn_left_handler(httpd_req_t *req) {
    sendMotorCommand('A');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t turn_right_handler(httpd_req_t *req) {
    sendMotorCommand('D');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t goleft_handler(httpd_req_t *req) {
    sendMotorCommand('Q');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t goright_handler(httpd_req_t *req) {
    sendMotorCommand('W');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t backleft_handler(httpd_req_t *req) {
    sendMotorCommand('Z');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t backright_handler(httpd_req_t *req) {
    sendMotorCommand('X');
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t ledon_handler(httpd_req_t *req) {
    digitalWrite(gpLed, HIGH);
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t ledoff_handler(httpd_req_t *req) {
    digitalWrite(gpLed, LOW);
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

// Uppercase command handlers (aliases)
static esp_err_t F_handler(httpd_req_t *req) { return go_handler(req); }
static esp_err_t B_handler(httpd_req_t *req) { return back_handler(req); }
static esp_err_t S_handler(httpd_req_t *req) { return stop_handler(req); }
static esp_err_t L_handler(httpd_req_t *req) { return left_handler(req); }
static esp_err_t R_handler(httpd_req_t *req) { return right_handler(req); }
static esp_err_t A_handler(httpd_req_t *req) { return turn_left_handler(req); }
static esp_err_t D_handler(httpd_req_t *req) { return turn_right_handler(req); }
static esp_err_t Q_handler(httpd_req_t *req) { return goleft_handler(req); }
static esp_err_t W_handler(httpd_req_t *req) { return goright_handler(req); }
static esp_err_t Z_handler(httpd_req_t *req) { return backleft_handler(req); }
static esp_err_t X_handler(httpd_req_t *req) { return backright_handler(req); }

void startCameraServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 40;
    
    // Define all URI handlers
    httpd_uri_t uris[] = {
        {"/", HTTP_GET, index_handler, NULL},
        {"/distance", HTTP_GET, distance_handler, NULL},
        {"/go", HTTP_GET, go_handler, NULL},
        {"/back", HTTP_GET, back_handler, NULL},
        {"/stop", HTTP_GET, stop_handler, NULL},
        {"/left", HTTP_GET, left_handler, NULL},
        {"/right", HTTP_GET, right_handler, NULL},
        {"/goleft", HTTP_GET, goleft_handler, NULL},
        {"/goright", HTTP_GET, goright_handler, NULL},
        {"/backleft", HTTP_GET, backleft_handler, NULL},
        {"/backright", HTTP_GET, backright_handler, NULL},
        {"/Turn_Left", HTTP_GET, turn_left_handler, NULL},
        {"/Turn_Right", HTTP_GET, turn_right_handler, NULL},
        {"/ledon", HTTP_GET, ledon_handler, NULL},
        {"/ledoff", HTTP_GET, ledoff_handler, NULL},
        // Single letter aliases
        {"/F", HTTP_GET, F_handler, NULL},
        {"/B", HTTP_GET, B_handler, NULL},
        {"/S", HTTP_GET, S_handler, NULL},
        {"/L", HTTP_GET, L_handler, NULL},
        {"/R", HTTP_GET, R_handler, NULL},
        {"/A", HTTP_GET, A_handler, NULL},
        {"/D", HTTP_GET, D_handler, NULL},
        {"/Q", HTTP_GET, Q_handler, NULL},
        {"/W", HTTP_GET, W_handler, NULL},
        {"/Z", HTTP_GET, Z_handler, NULL},
        {"/X", HTTP_GET, X_handler, NULL}
    };
    
    ra_filter_init(&ra_filter, 20);
    
    // Start main HTTP server on port 80
    if(httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        for(int i = 0; i < sizeof(uris)/sizeof(uris[0]); i++)
        {
            httpd_register_uri_handler(camera_httpd, &uris[i]);
        }
    }
    
    // Start stream server on port 81
    config.server_port = 81;
    config.ctrl_port = 81;
    
    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };
    
    if(httpd_start(&stream_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}
