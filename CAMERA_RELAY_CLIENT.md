# CameraRelayClient Module

**Complete ESP32-CAM to Relay Server Streaming Solution**

A fully integrated module that handles camera initialization, frame capture, and TCP streaming to a relay server. Everything you need in one simple class.

## ✨ Key Features

✅ **All-in-One Solution**: Camera init + frame capture + TCP streaming  
✅ **Ultra Simple**: Just call `run()` in loop - that's it!  
✅ **Non-Blocking**: Designed for smooth operation  
✅ **Auto-Reconnection**: Handles connection drops automatically  
✅ **FPS Control**: Built-in frame rate throttling  
✅ **Status Codes**: Know exactly what's happening  
✅ **Statistics**: Track frames, bytes, and actual FPS  
✅ **Zero Configuration**: Sensible defaults for ESP32-CAM

## 🚀 Quick Start

### Minimal Example

```cpp
#include <WiFi.h>
#include "secrets.h"
#include "CameraRelayClient.h"

CameraRelayClient camera("relay.example.com", 1234, 10.0);

void setup() {
  WiFi.begin(SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  camera.begin();
}

void loop() {
  camera.run();  // That's it!
}
```

### With Error Handling

```cpp
void loop() {
  CameraRelayClient::Status status = camera.run();
  
  if (status == CameraRelayClient::CAMERA_INIT_FAILED) {
    Serial.println("Camera initialization failed!");
    while(1) delay(1000);  // Halt
  }
}
```

## 📖 API Reference

### Constructor

```cpp
CameraRelayClient(const char* host, uint16_t port, float targetFPS = 10.0)
```

Creates a camera relay client instance.

**Parameters:**
- `host`: Relay server hostname or IP address
- `port`: Relay server port
- `targetFPS`: Target frames per second (default: 10.0)

**Example:**
```cpp
CameraRelayClient camera("192.168.1.100", 1234, 30.0);
```

---

### begin()

```cpp
bool begin()
```

Initializes the camera and prepares the client. Call in `setup()` after WiFi is connected.

**Returns:**
- `true` if initialization successful
- `false` if camera initialization failed

**Example:**
```cpp
void setup() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  if (!camera.begin()) {
    Serial.println("Camera init failed!");
    while(1);
  }
}
```

---

### run()

```cpp
Status run()
```

Main loop method - handles everything automatically! Call this repeatedly in `loop()`.

**Returns:** Status code (see Status enum below)

**What it does:**
1. Checks camera initialization
2. Manages connection to relay server
3. Captures frames at target FPS
4. Sends frames to relay server
5. Handles errors and reconnection

**Example:**
```cpp
void loop() {
  CameraRelayClient::Status status = camera.run();
  
  // Optional: React to specific statuses
  switch(status) {
    case CameraRelayClient::OK:
      // Frame sent successfully
      break;
    case CameraRelayClient::IDLE:
      // Waiting for next frame time
      break;
    // ... handle other cases ...
  }
}
```

---

### Status Enum

```cpp
enum Status {
    OK = 0,                    // Frame captured and sent successfully
    CAMERA_INIT_FAILED = 1,    // Camera initialization failed
    CAMERA_CAPTURE_FAILED = 2, // Frame capture failed
    NOT_CONNECTED = 3,         // Not connected to relay server
    SEND_FAILED = 4,           // Failed to send frame
    RECONNECTING = 5,          // Currently attempting to reconnect
    IDLE = 6                   // Waiting for next frame time (FPS throttling)
};
```

---

### isConnected()

```cpp
bool isConnected() const
```

Check if currently connected to relay server.

**Example:**
```cpp
if (camera.isConnected()) {
  Serial.println("Streaming...");
}
```

---

### isCameraReady()

```cpp
bool isCameraReady() const
```

Check if camera is initialized.

**Example:**
```cpp
if (!camera.isCameraReady()) {
  Serial.println("Camera not ready!");
}
```

---

### setTargetFPS() / getTargetFPS()

```cpp
void setTargetFPS(float fps)
float getTargetFPS() const
```

Set or get target frames per second.

**Example:**
```cpp
camera.setTargetFPS(30.0);  // Switch to 30 FPS
Serial.printf("FPS: %.1f\n", camera.getTargetFPS());
```

---

### Statistics Methods

```cpp
uint32_t getFrameCount() const   // Total frames sent
uint32_t getBytesSent() const    // Total bytes sent
float getActualFPS() const       // Actual FPS (measured over last 10 frames)
```

**Example:**
```cpp
Serial.printf("Frames: %u, Bytes: %u, FPS: %.1f\n",
              camera.getFrameCount(),
              camera.getBytesSent(),
              camera.getActualFPS());
```

---

### setRetryDelay()

```cpp
void setRetryDelay(unsigned long delayMs)
```

Set delay between reconnection attempts (default: 5000ms).

**Example:**
```cpp
camera.setRetryDelay(10000);  // Wait 10 seconds between retries
```

---

### setDebug()

```cpp
void setDebug(bool enable)
```

Enable/disable debug output to Serial.

**Example:**
```cpp
camera.setDebug(true);  // Enable debug output
```

---

### getStatusString()

```cpp
static const char* getStatusString(Status status)
```

Get human-readable status description.

**Example:**
```cpp
Status status = camera.run();
Serial.printf("Status: %s\n", CameraRelayClient::getStatusString(status));
```

---

### disconnect()

```cpp
void disconnect()
```

Manually disconnect from relay server.

**Example:**
```cpp
camera.disconnect();
```

---

## 🎯 Usage Patterns

### Pattern 1: Minimal Streaming

```cpp
CameraRelayClient camera("relay.server.com", 1234, 10.0);

void setup() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  camera.begin();
}

void loop() {
  camera.run();
}
```

### Pattern 2: With Status Monitoring

```cpp
void loop() {
  CameraRelayClient::Status status = camera.run();
  
  static CameraRelayClient::Status lastStatus = CameraRelayClient::IDLE;
  if (status != lastStatus) {
    Serial.printf("Status changed: %s\n", 
                  CameraRelayClient::getStatusString(status));
    lastStatus = status;
  }
}
```

### Pattern 3: Adaptive FPS

```cpp
void loop() {
  camera.run();
  
  // Reduce FPS if falling behind
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    float actualFPS = camera.getActualFPS();
    float targetFPS = camera.getTargetFPS();
    
    if (actualFPS < targetFPS * 0.8) {
      camera.setTargetFPS(targetFPS * 0.9);
      Serial.println("Reducing FPS due to performance");
    }
    
    lastCheck = millis();
  }
}
```

### Pattern 4: Periodic Statistics

```cpp
void loop() {
  camera.run();
  
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 10000) {
    Serial.printf("Frames: %u | Bytes: %u | FPS: %.1f\n",
                  camera.getFrameCount(),
                  camera.getBytesSent(),
                  camera.getActualFPS());
    lastStats = millis();
  }
}
```

### Pattern 5: Error Recovery

```cpp
void loop() {
  CameraRelayClient::Status status = camera.run();
  
  if (status == CameraRelayClient::CAMERA_CAPTURE_FAILED) {
    static int consecutiveFailures = 0;
    consecutiveFailures++;
    
    if (consecutiveFailures > 10) {
      Serial.println("Too many capture failures - restarting ESP32");
      ESP.restart();
    }
  } else {
    consecutiveFailures = 0;
  }
}
```

## 🔧 Configuration

### Camera Settings

The module uses optimized camera settings:

- **Resolution**: UXGA (1600x1200) with PSRAM, SVGA (800x600) without
- **Format**: JPEG
- **Quality**: 10 with PSRAM, 12 without
- **Frame Buffers**: 2 with PSRAM, 1 without
- **Grab Mode**: GRAB_LATEST with PSRAM for best performance

These settings provide a good balance between quality and performance. The camera is configured automatically in `begin()`.

### FPS Recommendations

- **High Quality**: 5-10 FPS (good for monitoring)
- **Smooth Motion**: 15-20 FPS (adequate for most use cases)
- **Real-time**: 30 FPS (requires good WiFi and relay server)

### Network Settings

- **Retry Delay**: Default 5000ms (5 seconds)
- **Connection**: Automatic reconnection on failure
- **Send Timeout**: Handled by WiFiClient defaults

## 📊 Performance

### Memory Usage

- **Class Size**: ~140 bytes
- **Frame Buffer**: Managed by ESP32 camera driver (in PSRAM if available)
- **No Dynamic Allocation**: All memory pre-allocated

### Typical Performance

With ESP32-CAM and good WiFi:

- **10 FPS**: ~40-50 KB/s (JPEG quality 10)
- **20 FPS**: ~80-100 KB/s
- **30 FPS**: ~120-150 KB/s

Actual performance depends on:
- Scene complexity (JPEG compression ratio)
- WiFi signal strength
- Relay server performance
- Network bandwidth

## 🐛 Troubleshooting

### Camera initialization fails

```
Status: CAMERA_INIT_FAILED
```

**Solutions:**
- Check camera is properly connected
- Verify correct board type (AI Thinker ESP32-CAM)
- Ensure PSRAM is enabled in platformio.ini
- Try power cycling the ESP32

### No connection to relay server

```
Status: RECONNECTING (continuous)
```

**Solutions:**
- Verify relay server is running
- Check host/port configuration
- Ensure firewall allows connection
- Verify ESP32 and relay are on same network (or properly routed)

### Low actual FPS

```
getActualFPS() much lower than getTargetFPS()
```

**Solutions:**
- Reduce JPEG quality (faster encoding)
- Lower resolution
- Reduce target FPS
- Check WiFi signal strength
- Verify relay server isn't bottleneck

### Frame capture failures

```
Status: CAMERA_CAPTURE_FAILED (intermittent)
```

**Solutions:**
- Usually temporary - module will retry
- If persistent, check camera connection
- Verify PSRAM settings
- May indicate hardware issue

## 🔄 Comparison: Before vs After

### Before (Manual Implementation)

```cpp
// ~130+ lines of code
#include "esp_camera.h"
#include "camera_config.h"
#include "RelayClient.h"

WiFiClient client;
RelayClient relay(host, port, fps);

void setup() {
  // Camera initialization
  bool cameraConfigured = createCameraConfiguration();
  if (!cameraConfigured) {
    Serial.println("ERROR: Camera failed!");
    return;
  }
  
  // WiFi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  // Relay client setup
  relay.begin();
}

void loop() {
  relay.run();
  
  if (relay.isConnected() && relay.canSend()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Failed to capture image");
      return;
    }
    
    if (!relay.sendData(fb->buf, fb->len)) {
      Serial.println("Failed to send frame");
    }
    
    esp_camera_fb_return(fb);
  }
}
```

### After (CameraRelayClient)

```cpp
// ~25 lines of code
#include "CameraRelayClient.h"

CameraRelayClient camera(host, port, fps);

void setup() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  camera.begin();
}

void loop() {
  camera.run();
}
```

**Benefits:**
- ✅ 80% less code
- ✅ No manual camera configuration
- ✅ No manual frame buffer management
- ✅ Built-in error handling
- ✅ Single point of configuration

## 🔗 Integration

### Use with relay_server_receiver.py

This module is designed to work with `relay_server_receiver.py` (push mode):

**On your VPS:**
```bash
python relay_server_receiver.py
# Listening for ESP32 on port 1234
# Serving clients on port 8080
```

**ESP32-CAM code:**
```cpp
CameraRelayClient camera("your.vps.ip", 1234, 10.0);
```

**Client connection:**
```bash
nc your.vps.ip 8080 > stream.mjpeg
```

### Multiple Cameras

Run multiple cameras pointing to the same relay server on different source ports:

```cpp
// Camera 1
CameraRelayClient camera1("relay.server", 1234, 10.0);

// Camera 2  
CameraRelayClient camera2("relay.server", 1235, 10.0);
```

On relay server, run multiple instances or modify to handle multiple source ports.

## 📝 License

Same as parent project.

## 🔗 See Also

- `TcpServer.h` - Server mode (relay pulls from ESP32)
- `RelayClient.h` - Generic TCP client (no camera coupling)
- `relay_server_receiver.py` - Relay server for push mode
- `relay_server.py` - Relay server for pull mode
- `main_camera_client.cpp` - Complete usage example

---

## 🎓 Summary

`CameraRelayClient` is the **easiest way** to stream ESP32-CAM video to a relay server:

1. Create instance with host/port/fps
2. Call `begin()` in setup
3. Call `run()` in loop
4. Done! ✨

Perfect for:
- Remote monitoring systems
- Security cameras
- IoT video streaming
- Webcam projects
- Live streaming applications
