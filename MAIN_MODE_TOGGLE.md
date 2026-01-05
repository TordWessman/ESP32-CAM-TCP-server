# Main.cpp Mode Toggle Guide

The `main.cpp` file now supports **easy toggling** between Server Mode (pull) and Client Mode (push) with just one line change!

## 🎯 Quick Mode Selection

Edit `src/main.cpp` and uncomment the mode you want:

```cpp
// ============================================================
// MODE SELECTION - Choose ONE by uncommenting
// ============================================================
#define USE_SERVER_MODE    // CameraTcpServer (relay pulls from ESP32)
// #define USE_CLIENT_MODE // CameraRelayClient (ESP32 pushes to relay)
// ============================================================
```

**That's it!** The code automatically includes the right module and uses the right configuration.

---

## 🔄 Mode Details

### Server Mode (Pull)

**Uncomment:**
```cpp
#define USE_SERVER_MODE
```

**What it does:**
- ESP32-CAM acts as TCP server
- Listens on port 1234
- Relay server connects TO ESP32
- Relay fetches frames from ESP32

**Network flow:**
```
Relay Server → connects to → ESP32-CAM (server)
```

**Relay server to use:** `relay_server.py`

**Configuration:**
```cpp
#define PORT 1234
#define FPS 30.0
```

**Module used:** `CameraTcpServer`

---

### Client Mode (Push)

**Uncomment:**
```cpp
#define USE_CLIENT_MODE
```

**What it does:**
- ESP32-CAM acts as TCP client
- Connects to relay server
- ESP32 pushes frames to relay

**Network flow:**
```
ESP32-CAM (client) → connects to → Relay Server
```

**Relay server to use:** `relay_server_receiver.py`

**Configuration:**
```cpp
#define RELAY_HOST "192.168.1.50"  // Your relay server IP
#define RELAY_PORT 1234
#define FPS 10.0
```

**Module used:** `CameraRelayClient`

---

## 📝 Complete main.cpp Overview

### Structure

```cpp
// 1. Mode selection (choose one)
#define USE_SERVER_MODE    // or USE_CLIENT_MODE

// 2. Conditional includes and configuration
#ifdef USE_SERVER_MODE
  #include "CameraTcpServer.h"
  // Server config
#elif defined(USE_CLIENT_MODE)
  #include "CameraRelayClient.h"
  // Client config
#endif

// 3. Unified setup()
void setup() {
  // WiFi connection
  // Camera initialization via camera.begin()
}

// 4. Unified loop()
void loop() {
  // Just calls camera.run()!
  // Statistics every 10 seconds
}
```

### Complete Code

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"

// ============================================================
// MODE SELECTION - Choose ONE by uncommenting
// ============================================================
#define USE_SERVER_MODE    // CameraTcpServer (relay pulls from ESP32)
// #define USE_CLIENT_MODE // CameraRelayClient (ESP32 pushes to relay)
// ============================================================

#ifdef USE_SERVER_MODE
  #include "CameraTcpServer.h"
  #define PORT 1234
  #define FPS 30.0
  CameraTcpServer camera(PORT, FPS);
  #define MODE_NAME "Server Mode (Pull)"
#elif defined(USE_CLIENT_MODE)
  #include "CameraRelayClient.h"
  #define RELAY_HOST "192.168.1.50"  // Change to your relay server IP
  #define RELAY_PORT 1234
  #define FPS 10.0
  CameraRelayClient camera(RELAY_HOST, RELAY_PORT, FPS);
  #define MODE_NAME "Client Mode (Push)"
#else
  #error "Please define either USE_SERVER_MODE or USE_CLIENT_MODE"
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=================================");
  Serial.printf("ESP32-CAM Streaming: %s\n", MODE_NAME);
  Serial.println("=================================");

  WiFi.begin(SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\n✓ WiFi connected!");
  Serial.print("ESP32-CAM IP: ");
  Serial.println(WiFi.localIP());

  camera.setDebug(true);

  if (!camera.begin()) {
    Serial.println("ERROR: Camera initialization failed!");
    Serial.println("System halted.");
    while(1) delay(1000);
  }

  Serial.println("\n=================================");
  Serial.println("✓ System ready!");
  
#ifdef USE_SERVER_MODE
  Serial.printf("Listening on port %d\n", PORT);
  Serial.println("Waiting for relay server to connect...");
#else
  Serial.printf("Relay Server: %s:%d\n", RELAY_HOST, RELAY_PORT);
  Serial.println("Will connect to relay server...");
#endif
  
  Serial.printf("Target FPS: %.1f\n", FPS);
  Serial.println("=================================\n");
}

void loop() {
  // Run camera streaming (everything handled internally!)
#ifdef USE_SERVER_MODE
  CameraTcpServer::Status status = camera.run();
#else
  CameraRelayClient::Status status = camera.run();
#endif

  // Optional: Print stats every 10 seconds
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 10000) {
    Serial.printf("\n--- Statistics ---\n");
    Serial.printf("Frames sent: %u\n", camera.getFrameCount());
    Serial.printf("Bytes sent: %u\n", camera.getBytesSent());
    Serial.printf("Actual FPS: %.1f\n", camera.getActualFPS());
    
#ifdef USE_SERVER_MODE
    Serial.printf("Clients served: %u\n", camera.getClientCount());
    Serial.printf("Status: %s\n", CameraTcpServer::getStatusString(status));
#else
    Serial.printf("Connected: %s\n", camera.isConnected() ? "Yes" : "No");
    Serial.printf("Status: %s\n", CameraRelayClient::getStatusString(status));
#endif
    
    Serial.printf("------------------\n\n");
    lastStats = millis();
  }
}
```

---

## 🚀 How to Switch Modes

### Step 1: Edit main.cpp

Open `src/main.cpp` and change the mode definition:

**For Server Mode (Pull):**
```cpp
#define USE_SERVER_MODE
// #define USE_CLIENT_MODE
```

**For Client Mode (Push):**
```cpp
// #define USE_SERVER_MODE
#define USE_CLIENT_MODE
```

### Step 2: Configure Settings

**Server Mode:**
- `PORT` - Port to listen on (default: 1234)
- `FPS` - Target frames per second (default: 30.0)

**Client Mode:**
- `RELAY_HOST` - Relay server IP or hostname
- `RELAY_PORT` - Relay server port
- `FPS` - Target frames per second (default: 10.0)

### Step 3: Upload

```bash
pio run --target upload
```

**Done!** The ESP32 will automatically work in the selected mode.

---

## 📊 Comparison Table

| Aspect | Server Mode | Client Mode |
|--------|-------------|-------------|
| **Module** | CameraTcpServer | CameraRelayClient |
| **ESP32 Role** | Server | Client |
| **Initiates Connection** | Relay | ESP32 |
| **Port Forwarding** | Required if behind NAT | Not required |
| **Relay Script** | relay_server.py | relay_server_receiver.py |
| **Default FPS** | 30.0 | 10.0 |
| **Network Config** | PORT | RELAY_HOST, RELAY_PORT |

---

## 🔧 Configuration Examples

### Server Mode Configuration

```cpp
#define USE_SERVER_MODE

// Server listens on port 8080 at 20 FPS
#define PORT 8080
#define FPS 20.0
```

Then configure relay_server.py:
```python
ESP32_HOST = "192.168.1.100"  # Your ESP32 IP
ESP32_PORT = 8080
```

### Client Mode Configuration

```cpp
#define USE_CLIENT_MODE

// Connect to relay server at example.com:9999 at 15 FPS
#define RELAY_HOST "relay.example.com"
#define RELAY_PORT 9999
#define FPS 15.0
```

Then run relay server:
```bash
python relay_server_receiver.py
# Listening on port 9999
```

---

## 🎯 Use Cases

### Use Server Mode When:

- ✅ ESP32 has a static IP or mDNS name
- ✅ You can configure port forwarding if needed
- ✅ You want relay server to control when frames are fetched
- ✅ You're on a local network
- ✅ You prefer pull-based architecture

### Use Client Mode When:

- ✅ ESP32 is behind NAT (no port forwarding possible)
- ✅ Relay server has a public IP/domain
- ✅ You want ESP32 to push frames continuously
- ✅ You prefer push-based architecture
- ✅ You want automatic reconnection from ESP32 side

---

## 🐛 Troubleshooting

### Compilation Error: "identifier camera is undefined"

**Cause:** Neither mode is defined or both are defined

**Solution:**
```cpp
// Make sure EXACTLY ONE is uncommented:
#define USE_SERVER_MODE    // This one
// #define USE_CLIENT_MODE // NOT this one
```

### Server Mode: No client connects

1. Check ESP32 IP address (printed on serial)
2. Verify relay_server.py has correct ESP32_HOST
3. Test connection: `nc <esp32-ip> 1234`
4. Check firewall on ESP32 network

### Client Mode: Can't connect to relay

1. Verify RELAY_HOST is correct
2. Check relay_server_receiver.py is running
3. Test connection: `nc <relay-host> <relay-port>`
4. Check firewall on relay server

---

## 📈 Statistics Output

Both modes provide the same statistics every 10 seconds:

```
--- Statistics ---
Frames sent: 3000
Bytes sent: 15000000
Actual FPS: 29.8
Clients served: 5        (Server mode)
Connected: Yes           (Client mode)
Status: OK
------------------
```

---

## 🔄 Switching Between Modes

You can switch modes anytime:

1. Edit `src/main.cpp`
2. Change the `#define` line
3. Save
4. Re-upload to ESP32
5. Done!

**No other code changes needed!**

---

## 💡 Tips

### Tip 1: Use Different FPS for Different Modes

Server mode can handle higher FPS:
```cpp
#ifdef USE_SERVER_MODE
  #define FPS 30.0   // High FPS for server
#else
  #define FPS 10.0   // Lower FPS for client
#endif
```

### Tip 2: Add Mode-Specific Features

```cpp
void loop() {
  camera.run();
  
#ifdef USE_SERVER_MODE
  // Server-specific code
  if (!camera.hasClient()) {
    // Do something when no client
  }
#else
  // Client-specific code
  if (!camera.isConnected()) {
    // Do something when disconnected
  }
#endif
}
```

### Tip 3: Quick Testing

Keep two versions:
- `main_server.cpp` with USE_SERVER_MODE
- `main_client.cpp` with USE_CLIENT_MODE

Copy the one you need to `main.cpp`

---

## 📚 Related Documentation

- [CameraTcpServer Documentation](CAMERA_TCP_SERVER.md)
- [CameraRelayClient Documentation](CAMERA_RELAY_CLIENT.md)
- [Module Comparison Guide](MODULE_COMPARISON.md)
- [Quick Start Guide](QUICKSTART.md)

---

## ✅ Summary

The unified `main.cpp` provides:

✅ **One-line mode switching**  
✅ **Automatic configuration**  
✅ **Unified API** (both use `camera.run()`)  
✅ **Compile-time mode selection**  
✅ **Clean, maintainable code**  
✅ **No code duplication**  

Just change one `#define` and you're ready to go! 🚀
