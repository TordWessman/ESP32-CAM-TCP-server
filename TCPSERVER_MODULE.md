# TcpServer Module Documentation

A fully decoupled, reusable TCP server module for ESP32 that serves streaming data to connecting clients with built-in FPS control and statistics tracking.

## Features

✅ **Non-blocking**: Handles connections without blocking main loop  
✅ **FPS Control**: Built-in throttling for consistent frame rates  
✅ **Zero Dependencies**: No camera or sensor code coupling  
✅ **Statistics**: Track clients, frames, bytes, and actual FPS  
✅ **Auto Management**: Handles client connections/disconnections automatically  
✅ **Lightweight**: Minimal memory footprint (~120 bytes)

## Quick Start

```cpp
#include "TcpServer.h"

TcpServer server(1234, 10.0);  // Port 1234, 10 FPS

void setup() {
  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  server.begin();
}

void loop() {
  server.run();  // Non-blocking!
  
  if (server.hasClient() && server.canSend()) {
    server.sendData(myData, dataSize);
  }
}
```

## API Reference

### Constructor

```cpp
TcpServer(uint16_t port, float targetFPS = 10.0)
```

Creates a TCP server instance.

**Parameters:**
- `port`: TCP port to listen on (1-65535)
- `targetFPS`: Target frames per second (default: 10.0)

**Example:**
```cpp
TcpServer server(1234, 30.0);  // Port 1234, 30 FPS
```

---

### begin()

```cpp
void begin()
```

Starts the TCP server. Must be called in `setup()` after WiFi is connected.

**Example:**
```cpp
void setup() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  server.begin();  // Start server
}
```

---

### run()

```cpp
void run()
```

Main server loop - handles incoming client connections. **Non-blocking!**

Must be called repeatedly in `loop()`.

**Example:**
```cpp
void loop() {
  server.run();  // Always call this
  // ... rest of code ...
}
```

---

### sendData()

```cpp
bool sendData(const uint8_t* data, size_t length)
```

Sends data to the connected client.

**Parameters:**
- `data`: Pointer to data buffer
- `length`: Size of data in bytes

**Returns:**
- `true` if data was sent successfully
- `false` if send failed or no client connected

**Example:**
```cpp
camera_fb_t *fb = esp_camera_fb_get();
if (server.sendData(fb->buf, fb->len)) {
  Serial.println("Frame sent!");
}
esp_camera_fb_return(fb);
```

---

### canSend()

```cpp
bool canSend()
```

Checks if enough time has passed to send the next frame (FPS throttling).

**Returns:**
- `true` if ready to send
- `false` if should wait

**Example:**
```cpp
if (server.canSend()) {
  // Time for next frame
  server.sendData(data, size);
}
```

---

### hasClient()

```cpp
bool hasClient()
```

Checks if a client is currently connected.

**Returns:**
- `true` if client connected
- `false` if no client

**Example:**
```cpp
if (server.hasClient()) {
  Serial.println("Client is connected");
}
```

---

### setTargetFPS()

```cpp
void setTargetFPS(float fps)
```

Changes the target frames per second.

**Parameters:**
- `fps`: New FPS value

**Example:**
```cpp
server.setTargetFPS(60.0);  // Switch to 60 FPS
```

---

### getTargetFPS()

```cpp
float getTargetFPS() const
```

Returns the current target FPS.

**Example:**
```cpp
Serial.printf("Target FPS: %.1f\n", server.getTargetFPS());
```

---

### Statistics Methods

```cpp
uint32_t getClientCount() const    // Total clients served
uint32_t getFrameCount() const     // Total frames sent
uint32_t getBytesSent() const      // Total bytes sent
float getActualFPS()               // Actual FPS (last 10 frames)
```

**Example:**
```cpp
Serial.printf("Stats - Clients: %u, Frames: %u, Bytes: %u, FPS: %.1f\n",
              server.getClientCount(),
              server.getFrameCount(),
              server.getBytesSent(),
              server.getActualFPS());
```

---

### disconnectClient()

```cpp
void disconnectClient()
```

Manually disconnects the current client.

**Example:**
```cpp
if (sessionTimeout()) {
  server.disconnectClient();
}
```

---

## Usage Patterns

### Pattern 1: Camera Streaming

```cpp
TcpServer server(1234, 30.0);

void loop() {
  server.run();
  
  if (server.hasClient() && server.canSend()) {
    camera_fb_t *fb = esp_camera_fb_get();
    server.sendData(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
}
```

### Pattern 2: Sensor Data

```cpp
TcpServer server(9999, 10.0);

void loop() {
  server.run();
  
  if (server.hasClient() && server.canSend()) {
    float temp = readTemperature();
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer), "TEMP:%.2f\n", temp);
    server.sendData((uint8_t*)buffer, len);
  }
}
```

### Pattern 3: Multiple Servers

```cpp
TcpServer videoServer(1234, 30.0);
TcpServer dataServer(1235, 10.0);

void loop() {
  videoServer.run();
  dataServer.run();
  
  if (videoServer.hasClient() && videoServer.canSend()) {
    // Send video frame
  }
  
  if (dataServer.hasClient() && dataServer.canSend()) {
    // Send sensor data
  }
}
```

## Technical Details

### Memory Usage

- **Class Size**: ~120 bytes per instance
- **Stack Usage**: Minimal (no large buffers)
- **No Dynamic Allocation**: All memory pre-allocated

### FPS Throttling

The module uses `canSend()` to enforce frame rate limits:

```cpp
frameInterval = 1000.0 / targetFPS  // milliseconds
```

Example intervals:
- 60 FPS → 16.7 ms
- 30 FPS → 33.3 ms
- 10 FPS → 100 ms

### Client Handling

- **One client at a time**: Single active connection
- **Auto-disconnect**: On send errors
- **New connections**: Automatically accepted when previous client disconnects

### Statistics Tracking

- **Client Count**: Total clients served since startup
- **Frame Count**: Total frames sent across all clients
- **Bytes Sent**: Cumulative data transmitted
- **Actual FPS**: Calculated over last 10 frames using circular buffer

## Performance Tips

1. **FPS Selection**
   - 30 FPS for smooth video
   - 10-15 FPS for adequate motion
   - 1-5 FPS for monitoring

2. **Network Optimization**
   - Use WiFi power management wisely
   - Consider JPEG quality vs frame rate trade-off
   - Monitor `getActualFPS()` to detect bottlenecks

3. **Error Handling**
   - Always check `sendData()` return value
   - Monitor actual vs target FPS
   - Use `disconnectClient()` for timeout handling

## Troubleshooting

### No client connects
- Check firewall settings
- Verify port is not already in use
- Ensure `server.begin()` is called after WiFi connects

### Low actual FPS
- Camera capture may be slow (reduce resolution/quality)
- WiFi bandwidth limited
- Processing taking too long in loop()

### Connection drops
- Network stability issues
- Client timeout
- Buffer overflow on client side

### High memory usage
- Not an issue with TcpServer (~120 bytes)
- Check camera buffer settings if using ESP32-CAM

## Comparison: Before vs After

### Before (Coupled)
```cpp
// 70+ lines, tightly coupled to WiFiServer
WiFiServer server(PORT);
WiFiClient client;

void loop() {
  client = server.available();
  if (client) {
    while (client.connected()) {
      // Manual timing, error handling, etc.
      camera_fb_t *fb = esp_camera_fb_get();
      client.write(fb->buf, fb->len);
      esp_camera_fb_return(fb);
      delay(1000.0 / FPS);
    }
    client.stop();
  }
}
```

### After (Modular)
```cpp
// ~20 lines, clean separation
TcpServer server(PORT, FPS);

void loop() {
  server.run();
  if (server.hasClient() && server.canSend()) {
    camera_fb_t *fb = esp_camera_fb_get();
    server.sendData(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
}
```

## Integration with RelayClient

Use both modules together for bidirectional communication:

```cpp
TcpServer server(1234, 30.0);     // Serve to relay_server.py
RelayClient client("relay", 1235); // Push to relay_server_receiver.py

void loop() {
  // Server mode (relay pulls from ESP32)
  server.run();
  if (server.hasClient() && server.canSend()) {
    camera_fb_t *fb = esp_camera_fb_get();
    server.sendData(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
  
  // Or client mode (ESP32 pushes to relay)
  client.run();
  if (client.canSend()) {
    camera_fb_t *fb = esp_camera_fb_get();
    client.sendData(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
}
```

## License

Same as parent project.

## See Also

- `RelayClient.h` - Complementary TCP client module
- `examples/TcpServer_Examples.cpp` - More usage examples
- `relay_server.py` - Python relay that connects to TcpServer
