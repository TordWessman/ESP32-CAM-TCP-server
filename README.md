# ESP32-CAM TCP Server Example

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