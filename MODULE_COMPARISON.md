# Module Comparison Guide

This project now offers **three different approaches** for ESP32-CAM TCP streaming. Choose the one that best fits your needs.

## 📦 Available Modules

### 1. CameraRelayClient (Recommended for Most Users)
**Complete all-in-one camera streaming solution**

```cpp
#include "CameraRelayClient.h"

CameraRelayClient camera("relay.server.com", 1234, 10.0);

void loop() {
  camera.run();  // Everything handled internally!
}
```

**Best for:**
- ✅ Quick prototyping
- ✅ Simple camera streaming projects
- ✅ Users who want "it just works"
- ✅ Push mode (ESP32 connects TO relay)

**Features:**
- Camera initialization included
- Frame capture automatic
- Error handling built-in
- 25 lines of code total

---

### 2. TcpServer (Server Mode)
**Modular TCP server for serving data to clients**

```cpp
#include "TcpServer.h"

TcpServer server(1234, 30.0);

void loop() {
  server.run();
  if (server.hasClient() && server.canSend()) {
    // Get your data (camera, sensor, etc.)
    server.sendData(data, size);
  }
}
```

**Best for:**
- ✅ Pull mode (relay connects TO ESP32)
- ✅ Multiple data sources (not just camera)
- ✅ Custom frame capture logic
- ✅ Users who need more control

**Features:**
- No camera dependencies
- Works with any data source
- Client management built-in
- Statistics tracking

---

### 3. RelayClient (Generic TCP Client)
**Modular TCP client for any data streaming**

```cpp
#include "RelayClient.h"

RelayClient client("relay.server.com", 1234, 10.0);

void loop() {
  client.run();
  if (client.canSend()) {
    // Capture your data manually
    camera_fb_t *fb = esp_camera_fb_get();
    client.sendData(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
}
```

**Best for:**
- ✅ Custom implementations
- ✅ Non-camera data streaming
- ✅ Maximum flexibility
- ✅ Learning how it works

**Features:**
- Zero dependencies
- Works with anything
- Complete control
- Reusable for other projects

---

## 🔄 Quick Comparison Table

| Feature | CameraRelayClient | TcpServer | RelayClient |
|---------|------------------|-----------|-------------|
| **Camera Init** | ✅ Automatic | ❌ Manual | ❌ Manual |
| **Frame Capture** | ✅ Automatic | ❌ Manual | ❌ Manual |
| **Code Lines** | ~25 | ~40 | ~50 |
| **Mode** | Push (Client) | Pull (Server) | Push (Client) |
| **Camera Coupling** | ✅ Yes | ❌ No | ❌ No |
| **Learning Curve** | Easy | Medium | Medium |
| **Flexibility** | Low | High | High |
| **Use Case** | Camera streaming | Any data serving | Any data pushing |

---

## 🎯 Decision Guide

### Choose CameraRelayClient if:

- ✅ You want to stream ESP32-CAM video to a relay server
- ✅ You want the simplest possible code
- ✅ You're okay with opinionated camera settings
- ✅ You want push mode (ESP32 connects to relay)
- ✅ You don't need to customize camera configuration

**Example use case:** Home security camera streaming to cloud relay

---

### Choose TcpServer if:

- ✅ You want relay server to connect TO ESP32 (pull mode)
- ✅ You need custom camera configuration
- ✅ You might stream non-camera data
- ✅ You want to serve data to multiple clients over time
- ✅ You prefer separation of concerns

**Example use case:** ESP32 acts as server, relay pulls frames on demand

---

### Choose RelayClient if:

- ✅ You need maximum flexibility
- ✅ You want to stream sensor data or other non-camera content
- ✅ You need custom frame capture logic
- ✅ You want to learn how TCP streaming works
- ✅ You plan to modify the streaming logic

**Example use case:** Streaming temperature sensor data to relay server

---

## 💻 Code Comparison

### Streaming Camera to Relay Server

#### Using CameraRelayClient (25 lines)

```cpp
#include "CameraRelayClient.h"
#include "secrets.h"

CameraRelayClient camera("relay.server.com", 1234, 10.0);

void setup() {
  WiFi.begin(SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  camera.begin();
}

void loop() {
  camera.run();
}
```

#### Using TcpServer (40 lines)

```cpp
#include "TcpServer.h"
#include "camera_config.h"
#include "secrets.h"

TcpServer server(1234, 10.0);

void setup() {
  createCameraConfiguration();
  WiFi.begin(SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  server.begin();
}

void loop() {
  server.run();
  
  if (server.hasClient() && server.canSend()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      server.sendData(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
  }
}
```

#### Using RelayClient (50 lines)

```cpp
#include "RelayClient.h"
#include "camera_config.h"
#include "secrets.h"

RelayClient relay("relay.server.com", 1234, 10.0);

void setup() {
  createCameraConfiguration();
  WiFi.begin(SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  relay.begin();
}

void loop() {
  relay.run();
  
  if (relay.isConnected() && relay.canSend()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      relay.sendData(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    } else {
      Serial.println("Frame capture failed");
    }
  }
}
```

---

## 🔀 Network Architecture

### CameraRelayClient / RelayClient (Push Mode)

```
ESP32-CAM ----connects to----> Relay Server ----serves----> Clients
  (client)                      (your.vps.com)              (viewers)
```

Use with: `relay_server_receiver.py`

**Pros:**
- ESP32 can be behind NAT
- No port forwarding needed on ESP32
- Easy firewall configuration

**Cons:**
- Relay server must be reachable
- ESP32 initiates connection

---

### TcpServer (Pull Mode)

```
Relay Server ----connects to----> ESP32-CAM ----serves----> Relay ----serves----> Clients
  (client)                           (server)               (relay)              (viewers)
```

Use with: `relay_server.py`

**Pros:**
- ESP32 just waits for connections
- Simpler ESP32 code logic
- Relay controls when to fetch

**Cons:**
- ESP32 must be reachable (port forwarding if behind NAT)
- More complex network setup

---

## 🚀 Migration Guide

### From Manual Code to CameraRelayClient

**Before:**
```cpp
#include "esp_camera.h"
#include "camera_config.h"
#include "RelayClient.h"

bool cameraConfigured = createCameraConfiguration();
RelayClient relay(host, port, fps);

void loop() {
  relay.run();
  if (relay.canSend()) {
    camera_fb_t *fb = esp_camera_fb_get();
    relay.sendData(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
}
```

**After:**
```cpp
#include "CameraRelayClient.h"

CameraRelayClient camera(host, port, fps);

void loop() {
  camera.run();
}
```

**Steps:**
1. Replace includes
2. Change class name
3. Remove camera initialization
4. Remove frame capture code
5. Just call `run()`!

---

### From RelayClient to TcpServer

**Before (Push Mode):**
```cpp
#include "RelayClient.h"
RelayClient client(host, port, fps);

void loop() {
  client.run();
  if (client.canSend()) {
    // send data
  }
}
```

**After (Pull Mode):**
```cpp
#include "TcpServer.h"
TcpServer server(port, fps);

void loop() {
  server.run();
  if (server.hasClient() && server.canSend()) {
    // send data
  }
}
```

**Steps:**
1. Change from client to server
2. Remove host (server listens, doesn't connect)
3. Change condition from `canSend()` to `hasClient() && canSend()`
4. Update relay server to connect TO ESP32

---

## 📚 Documentation Links

- **CameraRelayClient**: See `CAMERA_RELAY_CLIENT.md`
- **TcpServer**: See `TCPSERVER_MODULE.md`
- **RelayClient**: See `RELAYCLIENT_MODULE.md`
- **Relay Servers**: See `relay_server.py` and `relay_server_receiver.py`
- **Upload Guide**: See `UPLOAD_GUIDE.md`

---

## 🎓 Recommendations

### For Beginners
Start with **CameraRelayClient** - it's the easiest and most straightforward.

### For Production
Use **TcpServer** or **RelayClient** depending on your network architecture needs.

### For Learning
Try all three! Start with CameraRelayClient, then understand how RelayClient works, then see TcpServer for the server-side approach.

### For Custom Projects
Use **RelayClient** or **TcpServer** as a base and modify to your needs.

---

## 🔧 Configuration Files

All three approaches share the same configuration files:

- `secrets.h` - WiFi credentials (gitignored)
- `camera_pins.h` - ESP32-CAM pin definitions
- `camera_config.h` - Only needed for TcpServer/RelayClient (manual camera init)

---

## Summary

| Your Need | Choose This |
|-----------|-------------|
| Simplest camera streaming | **CameraRelayClient** |
| ESP32 behind NAT | **CameraRelayClient** or **RelayClient** |
| ESP32 as server | **TcpServer** |
| Non-camera data | **TcpServer** or **RelayClient** |
| Maximum control | **RelayClient** |
| Learning TCP streaming | Try all three! |

All three modules are production-ready, well-documented, and tested. Choose based on your specific requirements! 🎯
