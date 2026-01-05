/**
 * CameraRelayClient - Implementation
 * 
 * Complete camera streaming solution with automatic camera initialization,
 * frame capture, and relay server communication.
 */

#include "CameraRelayClient.h"
#include "camera_pins.h"
#include <stdarg.h>

CameraRelayClient::CameraRelayClient(const char* host, uint16_t port, float targetFPS)
    : _host(host),
      _port(port),
      _targetFPS(targetFPS),
      _retryDelay(5000),
      _debug(false),
      _isConnected(false),
      _cameraInitialized(false),
      _lastConnectionAttempt(0),
      _lastFrameTime(0),
      _frameInterval((unsigned long)(1000.0 / targetFPS)),
      _frameCount(0),
      _bytesSent(0),
      _fpsIndex(0)
{
    // Initialize FPS timestamp array
    for (int i = 0; i < 10; i++) {
        _fpsTimestamps[i] = 0;
    }
}

bool CameraRelayClient::begin() {
    debugPrint("[CameraRelayClient] Initializing...");
    
    // Initialize camera
    if (!initCamera()) {
        debugPrint("[CameraRelayClient] ERROR: Camera initialization failed!");
        return false;
    }
    
    debugPrintf("[CameraRelayClient] Camera initialized successfully");
    debugPrintf("[CameraRelayClient] Relay server: %s:%d", _host, _port);
    debugPrintf("[CameraRelayClient] Target FPS: %.1f (interval: %lu ms)", _targetFPS, _frameInterval);
    
    return true;
}

bool CameraRelayClient::initCamera() {
    if (_cameraInitialized) {
        return true;
    }
    
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
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    
    // Optimize settings based on PSRAM availability
    if (config.pixel_format == PIXFORMAT_JPEG) {
        if (psramFound()) {
            config.jpeg_quality = 10;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
        } else {
            config.frame_size = FRAMESIZE_SVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    } else {
        config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
        config.fb_count = 2;
#endif
    }
    
    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        debugPrintf("[CameraRelayClient] Camera init failed with error 0x%x", err);
        return false;
    }
    
    // Get sensor settings for additional configuration
    sensor_t * s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
        s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
        s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
        s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
        s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
        s->set_aec2(s, 0);           // 0 = disable , 1 = enable
        s->set_ae_level(s, 0);       // -2 to 2
        s->set_aec_value(s, 300);    // 0 to 1200
        s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
        s->set_agc_gain(s, 0);       // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
        s->set_bpc(s, 0);            // 0 = disable , 1 = enable
        s->set_wpc(s, 1);            // 0 = disable , 1 = enable
        s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
        s->set_lenc(s, 1);           // 0 = disable , 1 = enable
        s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
        s->set_vflip(s, 0);          // 0 = disable , 1 = enable
        s->set_dcw(s, 1);            // 0 = disable , 1 = enable
        s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
    }
    
    _cameraInitialized = true;
    return true;
}

CameraRelayClient::Status CameraRelayClient::run() {
    // Check camera initialization
    if (!_cameraInitialized) {
        return CAMERA_INIT_FAILED;
    }
    
    // Handle connection
    if (!_isConnected) {
        attemptConnection();
        if (!_isConnected) {
            return RECONNECTING;
        }
    }
    
    // Check if client is still connected
    if (_isConnected && !_client.connected()) {
        debugPrint("[CameraRelayClient] Connection lost");
        _isConnected = false;
        return NOT_CONNECTED;
    }
    
    // Check FPS throttling
    if (!canSend()) {
        return IDLE;
    }
    
    // Capture frame
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        debugPrint("[CameraRelayClient] ✗ Frame capture failed");
        return CAMERA_CAPTURE_FAILED;
    }
    
    // Send frame
    size_t written = _client.write(fb->buf, fb->len);
    
    if (written != fb->len) {
        debugPrintf("[CameraRelayClient] ✗ Send failed: %u/%u bytes", written, fb->len);
        esp_camera_fb_return(fb);
        _isConnected = false;
        _client.stop();
        return SEND_FAILED;
    }
    
    // Update statistics
    _bytesSent += written;
    _frameCount++;
    _lastFrameTime = millis();
    updateFPS();
    
    // Free frame buffer
    esp_camera_fb_return(fb);
    
    // Periodic stats output
    if (_debug && _frameCount % 100 == 0) {
        debugPrintf("[CameraRelayClient] Stats - Frames: %u, Bytes: %u, FPS: %.1f", 
                    _frameCount, _bytesSent, getActualFPS());
    }
    
    return OK;
}

void CameraRelayClient::attemptConnection() {
    unsigned long now = millis();
    
    // Respect retry delay
    if (now - _lastConnectionAttempt < _retryDelay) {
        return;
    }
    
    _lastConnectionAttempt = now;
    
    debugPrintf("[CameraRelayClient] Connecting to %s:%d...", _host, _port);
    
    if (_client.connect(_host, _port)) {
        _isConnected = true;
        debugPrint("[CameraRelayClient] ✓ Connected!");
    } else {
        debugPrint("[CameraRelayClient] ✗ Connection failed");
    }
}

bool CameraRelayClient::canSend() const {
    unsigned long now = millis();
    
    // Handle timer overflow
    if (now < _lastFrameTime) {
        return true;
    }
    
    return (now - _lastFrameTime) >= _frameInterval;
}

void CameraRelayClient::setTargetFPS(float fps) {
    _targetFPS = fps;
    _frameInterval = (unsigned long)(1000.0 / fps);
    debugPrintf("[CameraRelayClient] FPS changed to %.1f (interval: %lu ms)", fps, _frameInterval);
}

float CameraRelayClient::getActualFPS() const {
    if (_fpsTimestamps[9] == 0) {
        return 0.0;  // Not enough samples
    }
    
    unsigned long elapsed = _fpsTimestamps[_fpsIndex] - _fpsTimestamps[(_fpsIndex + 1) % 10];
    if (elapsed == 0) {
        return 0.0;
    }
    
    return 9000.0 / elapsed;  // 9 intervals between 10 samples
}

void CameraRelayClient::updateFPS() {
    _fpsTimestamps[_fpsIndex] = millis();
    _fpsIndex = (_fpsIndex + 1) % 10;
}

void CameraRelayClient::disconnect() {
    if (_isConnected) {
        _client.stop();
        _isConnected = false;
        debugPrint("[CameraRelayClient] Disconnected");
    }
}

const char* CameraRelayClient::getStatusString(Status status) {
    switch (status) {
        case OK: return "OK";
        case CAMERA_INIT_FAILED: return "CAMERA_INIT_FAILED";
        case CAMERA_CAPTURE_FAILED: return "CAMERA_CAPTURE_FAILED";
        case NOT_CONNECTED: return "NOT_CONNECTED";
        case SEND_FAILED: return "SEND_FAILED";
        case RECONNECTING: return "RECONNECTING";
        case IDLE: return "IDLE";
        default: return "UNKNOWN";
    }
}

void CameraRelayClient::debugPrint(const char* message) {
    if (_debug) {
        Serial.println(message);
    }
}

void CameraRelayClient::debugPrintf(const char* format, ...) {
    if (_debug) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.println(buffer);
    }
}
