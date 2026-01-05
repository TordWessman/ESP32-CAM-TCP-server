# 🎯 Quick Reference Guide

Choose your streaming approach and get started in minutes!

---

## Option 1: CameraRelayClient (Easiest!)

**When to use:** You want the simplest camera streaming solution  
**Mode:** Push (ESP32 connects to relay)

### Setup (3 steps):

```cpp
// 1. Include and create instance
#include "CameraRelayClient.h"
CameraRelayClient camera("relay.server.com", 1234, 10.0);

// 2. Initialize in setup()
void setup() {
  WiFi.begin(SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  camera.begin();
}

// 3. Run in loop()
void loop() {
  camera.run();  // Done!
}
```

**That's it! Everything else is automatic.**

📖 [Full Documentation →](CAMERA_RELAY_CLIENT.md)

---

## Option 2: TcpServer

**When to use:** You want server mode (relay connects TO ESP32)  
**Mode:** Pull (relay fetches from ESP32)

### Setup (4 steps):

```cpp
// 1. Include and create
#include "TcpServer.h"
#include "camera_config.h"
TcpServer server(1234, 10.0);

// 2. Initialize camera and server
void setup() {
  createCameraConfiguration();
  WiFi.begin(SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  server.begin();
}

// 3. Handle server and capture frames
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

📖 [Full Documentation →](TCPSERVER_MODULE.md)

---

## Option 3: RelayClient

**When to use:** You need flexibility or non-camera streaming  
**Mode:** Push (ESP32 connects to relay)

### Setup (4 steps):

```cpp
// 1. Include and create
#include "RelayClient.h"
#include "camera_config.h"
RelayClient client("relay.server.com", 1234, 10.0);

// 2. Initialize camera and client
void setup() {
  createCameraConfiguration();
  WiFi.begin(SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  client.begin();
}

// 3. Handle client and capture frames
void loop() {
  client.run();
  
  if (client.isConnected() && client.canSend()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      client.sendData(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
  }
}
```

📖 [Full Documentation →](RELAYCLIENT_MODULE.md)

---

## Quick Comparison

| Feature | CameraRelayClient | TcpServer | RelayClient |
|---------|------------------|-----------|-------------|
| **Lines of code** | ~25 | ~40 | ~50 |
| **Camera init** | ✅ Auto | ❌ Manual | ❌ Manual |
| **Frame capture** | ✅ Auto | ❌ Manual | ❌ Manual |
| **Mode** | Push | Pull | Push |
| **Simplicity** | ⭐⭐⭐ | ⭐⭐ | ⭐⭐ |
| **Flexibility** | ⭐ | ⭐⭐⭐ | ⭐⭐⭐ |

---

## Relay Server Setup

### For CameraRelayClient or RelayClient (Push Mode):

```bash
python relay_server_receiver.py
```

ESP32 connects TO relay, relay serves clients.

### For TcpServer (Pull Mode):

```bash
python relay_server.py
```

Relay connects TO ESP32, fetches frames, serves clients.

---

## Network Architecture

### Push Mode (CameraRelayClient/RelayClient)
```
┌──────────┐   connects to   ┌─────────────┐   serves   ┌─────────┐
│ ESP32-CAM│─────────────────>│Relay Server │<───────────│ Clients │
└──────────┘                  └─────────────┘            └─────────┘
   (client)                      (server)                 (viewers)
```

**Pros:** ESP32 can be behind NAT, no port forwarding

### Pull Mode (TcpServer)
```
┌─────────────┐   connects to   ┌──────────┐
│Relay Server │<─────────────────│ ESP32-CAM│
└─────────────┘                  └──────────┘
      ↓ serves                      (server)
┌─────────┐
│ Clients │
└─────────┘
 (viewers)
```

**Pros:** ESP32 acts as simple server, relay controls when to fetch

---

## Common Configuration

All approaches use the same WiFi setup:

```cpp
// secrets.h
#define SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// In your code
WiFi.begin(SSID, WIFI_PASSWORD);
while (WiFi.status() != WL_CONNECTED) delay(500);
```

---

## Viewing the Stream

### Option 1: Test Client (Python)
```bash
python test_client.py relay.server.com 8080
```

### Option 2: netcat + VLC
```bash
nc relay.server.com 8080 > stream.mjpeg
vlc stream.mjpeg
```

### Option 3: ffplay
```bash
nc relay.server.com 8080 | ffplay -
```

---

## Typical FPS Settings

| FPS | Use Case | Bandwidth |
|-----|----------|-----------|
| 5 | Monitoring, low bandwidth | ~20-30 KB/s |
| 10 | Standard surveillance | ~40-50 KB/s |
| 15 | Smooth motion | ~60-80 KB/s |
| 20 | High quality motion | ~80-100 KB/s |
| 30 | Real-time streaming | ~120-150 KB/s |

---

## Troubleshooting Quick Tips

### ❌ Camera Init Failed
- Check camera connection
- Verify PSRAM enabled in platformio.ini
- Power cycle ESP32

### ❌ Won't Connect to Relay
- Verify relay server is running
- Check host/port configuration
- Test network connectivity
- Check firewall rules

### ❌ Low FPS
- Reduce JPEG quality
- Lower resolution  
- Check WiFi signal
- Reduce target FPS

### ❌ Upload Failed
- Connect GPIO0 to GND before upload
- Disconnect GPIO0 after upload
- See [UPLOAD_GUIDE.md](UPLOAD_GUIDE.md)

---

## Decision Tree

```
Need to stream ESP32-CAM?
│
├─ Want simplest solution?
│  └─ ✅ Use CameraRelayClient
│
├─ Want server mode (relay pulls)?
│  └─ ✅ Use TcpServer
│
├─ Need maximum flexibility?
│  └─ ✅ Use RelayClient
│
└─ Not using camera?
   └─ ✅ Use TcpServer or RelayClient
```

---

## Example Files

- **`src/main_camera_client.cpp`** - CameraRelayClient example
- **`src/main.cpp`** - TcpServer example
- **`src/main_client.cpp`** - RelayClient example
- **`examples/TcpServer_Examples.cpp`** - 8 server patterns
- **`examples/RelayClient_Examples.cpp`** - 8 client patterns

---

## Full Documentation

- 📖 [CameraRelayClient Documentation](CAMERA_RELAY_CLIENT.md)
- 📖 [TcpServer Documentation](TCPSERVER_MODULE.md)
- 📖 [RelayClient Documentation](RELAYCLIENT_MODULE.md)
- 📖 [Module Comparison Guide](MODULE_COMPARISON.md)
- 📖 [Upload Instructions](UPLOAD_GUIDE.md)

---

## 🚀 Get Started Now!

1. Choose your approach (CameraRelayClient recommended)
2. Copy the code example above
3. Set WiFi credentials in `secrets.h`
4. Configure relay server address
5. Upload and run!

**That's it! You're streaming!** 🎉
