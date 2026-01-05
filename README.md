# ESP32-CAM TCP Streaming

Professional ESP32-CAM streaming solution with **three modular approaches** for different use cases. Stream camera video to local network or cloud relay servers with automatic reconnection, FPS control, and statistics tracking.

## 🚀 Quick Start (Easiest Method)

### Unified main.cpp - One File, Two Modes!

The simplest way to get started - just toggle between modes with one line:

```cpp
// In src/main.cpp - Choose your mode:
#define USE_SERVER_MODE    // CameraTcpServer (relay pulls from ESP32)
// #define USE_CLIENT_MODE // CameraRelayClient (ESP32 pushes to relay)

void setup() {
  WiFi.begin(SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  camera.begin();
}

void loop() {
  camera.run();  // That's it!
}
```

**Just change one `#define` to switch between server and client modes!**

📖 **[Mode Toggle Guide →](MAIN_MODE_TOGGLE.md)**

---

### Individual Modules

Or use the modules individually:

#### CameraRelayClient (Push Mode)
```cpp
#include "CameraRelayClient.h"
CameraRelayClient camera("relay.server.com", 1234, 10.0);

void loop() {
  camera.run();
}
```

#### CameraTcpServer (Pull Mode)
```cpp
#include "CameraTcpServer.h"
CameraTcpServer camera(1234, 30.0);

void loop() {
  camera.run();
}
```

---

## 📦 Available Modules

This project offers **two integrated camera modules** plus generic building blocks:

### Camera-Integrated Modules (Recommended)

#### 1. **CameraRelayClient** (Client/Push Mode)
- ✅ ESP32 pushes frames to relay server
- ✅ Works behind NAT (no port forwarding)
- ✅ Complete all-in-one solution
- ✅ Just call `camera.run()` in loop
- 📖 [Documentation](CAMERA_RELAY_CLIENT.md)

#### 2. **CameraTcpServer** (Server/Pull Mode)
- ✅ ESP32 acts as TCP server
- ✅ Relay pulls frames from ESP32
- ✅ Complete all-in-one solution
- ✅ Just call `camera.run()` in loop
- 📖 [Documentation](CAMERA_TCP_SERVER.md)

### Generic Building Blocks

#### 3. **TcpServer** (No Camera Coupling)
- ✅ Works with any data source
- ✅ Good separation of concerns
- ✅ Maximum flexibility
- 📖 [Documentation](TCPSERVER_MODULE.md)

#### 4. **RelayClient** (No Camera Coupling)
- ✅ Reusable for any TCP streaming
- ✅ Sensor data, telemetry, etc.
- ✅ Educational
- 📖 [Documentation](RELAYCLIENT_MODULE.md)

**Not sure which to choose?** See [Module Comparison Guide](MODULE_COMPARISON.md)

---

## ESP32-CAM TCP Server Example

Simple example of an ESP32-CAM application that streams JPEG encoded (ov2640) video from a TCP server endpoint.  

I selected the board _AI Thinker ESP32-CAM_.

## Setup

### WiFi Configuration

1. Copy `include/secrets.h.example` to `include/secrets.h`:
   ```bash
   cp include/secrets.h.example include/secrets.h
   ```

2. Edit `include/secrets.h` and set your WiFi credentials:
   ```cpp
   #define SSID "your-wifi-ssid"
   #define WIFI_PASSWORD "your-wifi-password"
   ```

**Note:** The `secrets.h` file is ignored by git to keep your credentials private.

### Building and Running

The debug output will print out the server's address.

---

**For PlatformIO users:** See [README_PLATFORMIO.md](README_PLATFORMIO.md) and [PLATFORMIO_QUICKSTART.md](PLATFORMIO_QUICKSTART.md) for detailed setup instructions.

---

## 📁 Project Structure

```
camera.tcp/
├── include/
│   ├── CameraRelayClient.h      # Camera + TCP client (push)
│   ├── CameraTcpServer.h        # Camera + TCP server (pull)
│   ├── TcpServer.h              # Generic TCP server
│   ├── RelayClient.h            # Generic TCP client
│   ├── camera_config.h          # Camera configuration
│   ├── camera_pins.h            # ESP32-CAM pin definitions
│   └── secrets.h                # WiFi credentials (gitignored)
├── src/
│   ├── CameraRelayClient.cpp    # Implementation
│   ├── CameraTcpServer.cpp      # Implementation
│   ├── TcpServer.cpp            # Implementation
│   ├── RelayClient.cpp          # Implementation
│   ├── main.cpp                 # Unified - toggle between modes!
│   ├── main_client.cpp          # Generic RelayClient example
│   └── main_camera_client.cpp   # CameraRelayClient example
├── examples/
│   ├── TcpServer_Examples.cpp   # Server usage examples
│   └── RelayClient_Examples.cpp # Client usage examples
├── relay_server.py              # Python relay (pull mode)
├── relay_server_receiver.py     # Python relay (push mode)
├── test_client.py               # Test client for viewing stream
├── CAMERA_RELAY_CLIENT.md       # CameraRelayClient docs
├── CAMERA_TCP_SERVER.md         # CameraTcpServer docs
├── TCPSERVER_MODULE.md          # TcpServer docs
├── RELAYCLIENT_MODULE.md        # RelayClient docs
├── MAIN_MODE_TOGGLE.md          # main.cpp mode switching guide
├── MODULE_COMPARISON.md         # Choose the right module
└── UPLOAD_GUIDE.md              # ESP32 upload instructions
```

## 🛠️ Features

- ✅ **Multiple Implementations**: Choose server or client mode
- ✅ **Auto-Reconnection**: Handles connection drops gracefully
- ✅ **FPS Control**: Built-in frame rate throttling
- ✅ **Statistics**: Track frames sent, bytes, actual FPS
- ✅ **Non-Blocking**: Smooth operation without delays
- ✅ **Modular Design**: Reusable components
- ✅ **Well Documented**: Comprehensive guides and examples
- ✅ **Production Ready**: Error handling and status codes

## 🌐 Network Modes

### Push Mode (ESP32 → Relay → Clients)
ESP32 connects to relay server, relay serves to clients.

```
ESP32-CAM --push--> Relay Server <--pull-- Clients
```

- Use **CameraRelayClient** or **RelayClient**
- Run `relay_server_receiver.py` on your VPS
- ESP32 can be behind NAT (no port forwarding)

### Pull Mode (Relay → ESP32 → Relay → Clients)
Relay connects to ESP32, fetches frames, serves to clients.

```
Relay Server --pull--> ESP32-CAM
     ↓
  Clients
```

- Use **TcpServer**
- Run `relay_server.py` on your VPS
- ESP32 must be reachable (requires port forwarding if behind NAT)

## 📊 Performance

Typical performance with AI Thinker ESP32-CAM:

| FPS | Bandwidth | Use Case |
|-----|-----------|----------|
| 5-10 | ~40-50 KB/s | Monitoring, surveillance |
| 15-20 | ~80-100 KB/s | Smooth motion capture |
| 30 | ~120-150 KB/s | Real-time streaming |

*Actual performance depends on scene complexity, WiFi quality, and network conditions.*

## 🔧 Development

### Switching Between Modes

**For Server Mode (TcpServer):**
- Keep `src/main.cpp` active
- Connects with `relay_server.py`

**For Client Mode (RelayClient):**
- Rename `main_client.cpp` to `main.cpp`
- Connects with `relay_server_receiver.py`

**For Camera Client (CameraRelayClient):**
- Rename `main_camera_client.cpp` to `main.cpp`
- Connects with `relay_server_receiver.py`

### Building with PlatformIO

```bash
# Build
pio run

# Upload (with GPIO0 to GND for programming mode)
pio run --target upload

# Monitor serial output
pio device monitor
```

See [UPLOAD_GUIDE.md](UPLOAD_GUIDE.md) for detailed upload instructions.

## 🐛 Troubleshooting

### Upload Issues
See [UPLOAD_GUIDE.md](UPLOAD_GUIDE.md) for GPIO0 programming mode instructions.

### No Serial Output
- Ensure GPIO0 is disconnected from GND after upload
- Add delay in setup() for serial monitor to connect
- Check baud rate (115200)

### Camera Init Failed
- Verify camera connection
- Check PSRAM enabled in platformio.ini
- Ensure correct board type selected

### Connection Issues
- Verify WiFi credentials in `secrets.h`
- Check relay server is running and reachable
- Ensure firewall allows the connection
- Verify correct host/port configuration

## 📚 Documentation

- **[CameraRelayClient Guide](CAMERA_RELAY_CLIENT.md)** - All-in-one streaming solution
- **[TcpServer Guide](TCPSERVER_MODULE.md)** - Server mode documentation
- **[RelayClient Guide](RELAYCLIENT_MODULE.md)** - Generic client documentation
- **[Module Comparison](MODULE_COMPARISON.md)** - Choose the right approach
- **[PlatformIO Setup](README_PLATFORMIO.md)** - Detailed PlatformIO guide
- **[Upload Guide](UPLOAD_GUIDE.md)** - ESP32 programming instructions

## 🎓 Examples

Each module includes comprehensive examples:

- **`examples/TcpServer_Examples.cpp`** - 8 server usage patterns
- **`examples/RelayClient_Examples.cpp`** - 8 client usage patterns
- **`src/main_camera_client.cpp`** - Complete CameraRelayClient example

## 📝 License

[Insert your license here]

## 🤝 Contributing

Contributions welcome! Please ensure:
- Code follows existing style
- Documentation is updated
- Examples are provided for new features

## ✨ Credits

Original TCP server example by TordWessman  
Modular components and relay server implementation added January 2026

---

**Ready to start streaming?** Choose your module from the [Module Comparison Guide](MODULE_COMPARISON.md) and check the respective documentation! 🚀