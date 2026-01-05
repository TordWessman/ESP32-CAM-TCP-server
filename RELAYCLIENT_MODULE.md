# RelayClient Module Documentation

## Overview

`RelayClient` is a reusable, non-blocking TCP client module for ESP32 that streams data to a relay server. It's completely decoupled from camera code and can be used for any data streaming purpose.

## Features

✅ **Non-blocking** - Safe to use in `loop()` without delays  
✅ **Auto-reconnection** - Automatically handles connection failures  
✅ **FPS throttling** - Built-in rate limiting  
✅ **Camera-independent** - No direct dependencies on ESP camera  
✅ **Statistics tracking** - Monitor frames sent, bytes, and actual FPS  
✅ **Configurable** - Retry delay, debug output, FPS control  
✅ **Simple API** - Easy to integrate into existing projects  

---

## Quick Start

### Basic Usage

```cpp
#include "RelayClient.h"

// Create client
RelayClient relay("192.168.1.50", 1234, 10.0);  // host, port, fps

void setup() {
    relay.begin();
    relay.setDebug(true);
}

void loop() {
    relay.run();  // Non-blocking, call every iteration
    
    if (relay.isConnected() && relay.canSend()) {
        uint8_t data[] = {1, 2, 3, 4, 5};
        relay.sendData(data, sizeof(data));
    }
}
```

### With ESP32-CAM

```cpp
#include "RelayClient.h"
#include "esp_camera.h"

RelayClient relay("relay.example.com", 1234, 15.0);

void setup() {
    // Initialize WiFi...
    // Initialize camera...
    
    relay.begin();
}

void loop() {
    relay.run();
    
    if (relay.isConnected() && relay.canSend()) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            relay.sendData(fb->buf, fb->len);
            esp_camera_fb_return(fb);
        }
    }
}
```

---

## API Reference

### Constructor

```cpp
RelayClient(const char* host, uint16_t port, float targetFPS = 0)
```

- `host` - Relay server hostname or IP address
- `port` - Relay server port number
- `targetFPS` - Target frames per second (0 = no limit, default)

**Example:**
```cpp
RelayClient relay1("192.168.1.50", 1234);        // No FPS limit
RelayClient relay2("relay.local", 5678, 30.0);   // 30 FPS limit
```

### Core Methods

#### `void begin()`

Initialize the relay client. Call once in `setup()`.

```cpp
relay.begin();
```

#### `void run()`

**Must call in every `loop()` iteration!** This is the heart of the non-blocking design.

Handles:
- Connection state monitoring
- Automatic reconnection attempts
- Connection timeout detection

```cpp
void loop() {
    relay.run();  // Essential!
    // ... rest of code
}
```

#### `bool sendData(const uint8_t* data, size_t length)`

Send data to relay server.

Returns:
- `true` if data sent successfully
- `false` if send failed or not connected

```cpp
uint8_t buffer[1024];
size_t len = 1024;
bool success = relay.sendData(buffer, len);
```

**Note:** Automatically closes connection after sending (relay server protocol).

### Status Methods

#### `bool isConnected()`

Check if currently connected to relay server.

```cpp
if (relay.isConnected()) {
    // Safe to send
}
```

#### `bool canSend()`

Check if ready to send next frame (respects FPS throttling).

```cpp
if (relay.canSend()) {
    // Time to send next frame
}
```

**Best practice:** Check both before sending:
```cpp
if (relay.isConnected() && relay.canSend()) {
    relay.sendData(data, len);
}
```

### Configuration Methods

#### `void setTargetFPS(float fps)`

Dynamically change target FPS.

```cpp
relay.setTargetFPS(30.0);  // 30 FPS
relay.setTargetFPS(0);     // No limit (as fast as possible)
```

#### `void setRetryDelay(unsigned long delayMs)`

Set delay between reconnection attempts (default: 5000ms).

```cpp
relay.setRetryDelay(3000);  // Retry every 3 seconds
```

#### `void setDebug(bool enable)`

Enable/disable debug output to Serial.

```cpp
relay.setDebug(true);   // Enable debug messages
relay.setDebug(false);  // Quiet mode
```

#### `void disconnect()`

Manually disconnect from server.

```cpp
relay.disconnect();
```

### Statistics Methods

#### `unsigned long getFrameCount()`

Get total number of frames sent.

```cpp
Serial.printf("Frames: %lu\n", relay.getFrameCount());
```

#### `unsigned long getTotalBytesSent()`

Get total bytes sent.

```cpp
Serial.printf("Bytes: %lu MB\n", relay.getTotalBytesSent() / 1024 / 1024);
```

#### `float getActualFPS()`

Get actual measured FPS (not target FPS).

```cpp
Serial.printf("Actual FPS: %.2f\n", relay.getActualFPS());
```

---

## Usage Patterns

### Pattern 1: Simple Streaming

```cpp
RelayClient relay("192.168.1.50", 1234);

void loop() {
    relay.run();
    
    if (relay.isConnected()) {
        // Get data from somewhere
        uint8_t data[100];
        fillDataBuffer(data);
        
        relay.sendData(data, sizeof(data));
    }
}
```

### Pattern 2: With FPS Control

```cpp
RelayClient relay("192.168.1.50", 1234, 10.0);  // 10 FPS

void loop() {
    relay.run();
    
    // Only sends when FPS throttling allows
    if (relay.canSend()) {
        getData();
        relay.sendData(buffer, len);
    }
}
```

### Pattern 3: Conditional Sending

```cpp
void loop() {
    relay.run();
    
    if (motionDetected && relay.canSend()) {
        // Only send when motion detected
        captureFrame();
        relay.sendData(frame, frameSize);
    }
}
```

### Pattern 4: Statistics Monitoring

```cpp
unsigned long lastStats = 0;

void loop() {
    relay.run();
    
    // Send data...
    
    // Print stats every 10 seconds
    if (millis() - lastStats > 10000) {
        Serial.printf("Frames: %lu, FPS: %.2f\n",
                     relay.getFrameCount(),
                     relay.getActualFPS());
        lastStats = millis();
    }
}
```

### Pattern 5: Error Recovery

```cpp
void loop() {
    relay.run();
    
    if (relay.isConnected() && relay.canSend()) {
        if (!relay.sendData(data, len)) {
            // Send failed - log it
            Serial.println("Send failed, will auto-reconnect");
            // No manual intervention needed!
        }
    }
}
```

---

## Implementation Details

### Non-Blocking Design

The `run()` method:
1. Checks connection status
2. Attempts reconnection if needed (with delay)
3. Returns immediately (never blocks)

### FPS Throttling

When `targetFPS > 0`:
- Calculates minimum interval between frames
- `canSend()` returns false if too soon
- Automatically enforces rate limit

### Connection Management

- Automatically attempts reconnection on failure
- Respects retry delay to avoid spam
- Closes connection after each send (protocol requirement)

### Thread Safety

⚠️ **Not thread-safe!** Use from one task only (typically `loop()`).

---

## Configuration Examples

### High Performance

```cpp
RelayClient relay("relay.local", 1234, 30.0);
relay.setRetryDelay(1000);  // Fast retry
relay.begin();
```

### Low Bandwidth

```cpp
RelayClient relay("relay.local", 1234, 1.0);  // 1 FPS
relay.setRetryDelay(10000);  // Slow retry
relay.begin();
```

### Burst Mode

```cpp
RelayClient relay("relay.local", 1234, 0);  // No FPS limit
relay.begin();
```

### Debug Mode

```cpp
RelayClient relay("relay.local", 1234, 10.0);
relay.setDebug(true);  // See all connection events
relay.begin();
```

---

## Common Patterns

### Camera Streaming

See `src/main_client.cpp` for complete example.

### Sensor Data Logger

```cpp
RelayClient logger("data.server.com", 1234, 1.0);  // 1 sample/sec

void loop() {
    logger.run();
    
    if (logger.canSend()) {
        float temp = readTemperature();
        logger.sendData((uint8_t*)&temp, sizeof(temp));
    }
}
```

### Multi-Stream

```cpp
RelayClient stream1("relay1.com", 1234, 30.0);  // High FPS
RelayClient stream2("relay2.com", 5678, 5.0);   // Low FPS

void loop() {
    stream1.run();
    stream2.run();
    
    if (stream1.canSend()) {
        stream1.sendData(highQualityData, len1);
    }
    if (stream2.canSend()) {
        stream2.sendData(lowQualityData, len2);
    }
}
```

---

## Troubleshooting

### No connection

**Check:**
- Relay server is running
- Host and port are correct
- WiFi is connected
- Firewall allows connection

**Enable debug:**
```cpp
relay.setDebug(true);
```

### FPS too low

**Check:**
- Target FPS setting
- Network bandwidth
- Data size
- Relay server processing speed

### Connection keeps dropping

**Try:**
```cpp
relay.setRetryDelay(1000);  // Faster reconnect
```

### High memory usage

- RelayClient itself is lightweight (~100 bytes)
- Main memory usage is your data buffer
- WiFiClient uses internal buffers

---

## File Structure

```
include/
  └── RelayClient.h        # Header file
src/
  └── RelayClient.cpp      # Implementation
  └── main_client.cpp      # Example usage with camera
examples/
  └── RelayClient_Examples.cpp  # More examples
```

---

## Dependencies

- Arduino.h
- WiFiClient.h (ESP32 WiFi library)

**No camera dependencies!** Can be used for any TCP streaming.

---

## Integration Checklist

- [ ] Add `#include "RelayClient.h"` to your code
- [ ] Create `RelayClient` instance (global or class member)
- [ ] Call `begin()` in `setup()`
- [ ] Call `run()` in every `loop()` iteration
- [ ] Check `isConnected()` and `canSend()` before sending
- [ ] Use `sendData()` to transmit data

---

## Performance Notes

- **CPU**: Minimal impact, non-blocking
- **Memory**: ~100 bytes + WiFiClient overhead
- **Network**: Depends on your data rate
- **Recommended FPS**: 5-30 for camera streams

---

## License

Part of ESP32-CAM TCP Server project.

---

## Support

For issues or questions, check:
- `examples/RelayClient_Examples.cpp` for usage patterns
- `src/main_client.cpp` for camera integration
- Enable debug mode for troubleshooting
