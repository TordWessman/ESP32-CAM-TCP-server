# ESP32-CAM TCP Server Example (PlatformIO)

Simple example of an ESP32-CAM application that streams JPEG encoded (ov2640) video from a TCP server endpoint.  

This project has been configured to work with **PlatformIO** for ESP32-CAM development boards.

## Hardware

This project is configured for the **AI Thinker ESP32-CAM** board, but supports other ESP32-CAM variants by modifying the camera model definition in `include/camera_config.h`.

## Setup

### Prerequisites

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)

### Configuration

1. **Set up WiFi credentials:**
   
   Copy the example secrets file:
   ```bash
   cp include/secrets.h.example include/secrets.h
   ```
   
   Then edit `include/secrets.h` and set your WiFi credentials:
   ```cpp
   #define SSID "your-wifi-ssid"
   #define WIFI_PASSWORD "your-wifi-password"
   ```
   
   **Note:** The `secrets.h` file is ignored by git to keep your credentials private.

2. (Optional) Adjust the TCP port and FPS in `src/main.cpp`:
   ```cpp
   #define PORT 1234
   #define FPS 30.0
   ```

### Building and Uploading

1. Open the project folder in VS Code
2. PlatformIO will automatically detect the project
3. Connect your ESP32-CAM board via USB (you may need an FTDI adapter)
4. Click the PlatformIO upload button or run:
   ```bash
   pio run --target upload
   ```

5. Open the Serial Monitor to see the server's IP address:
   ```bash
   pio device monitor
   ```

## Usage

Once the device connects to WiFi, it will print its IP address in the serial monitor. You can connect to it using any TCP client on `<IP_ADDRESS>:1234` (or your configured port).

Example using netcat:
```bash
nc <ESP32_IP_ADDRESS> 1234 > stream.jpg
```

## Project Structure

```
camera.tcp/
├── platformio.ini          # PlatformIO configuration
├── src/
│   └── main.cpp           # Main application code
├── include/
│   ├── camera_config.h    # Camera configuration
│   └── camera_pins.h      # Pin definitions for various ESP32-CAM boards
└── README.md
```

## Supported Camera Models

The project supports multiple ESP32-CAM board variants. You can select your board by modifying the `#define` in `include/camera_config.h`:

- `CAMERA_MODEL_AI_THINKER` (default)
- `CAMERA_MODEL_WROVER_KIT`
- `CAMERA_MODEL_ESP_EYE`
- `CAMERA_MODEL_M5STACK_PSRAM`
- And more (see `camera_pins.h`)

## Troubleshooting

- **Upload fails**: Make sure to put the ESP32-CAM into programming mode (connect GPIO0 to GND during power-up/reset)
- **Camera init failed**: Check that the correct camera model is defined in `camera_config.h`
- **Out of memory**: The project uses PSRAM. Make sure your board has PSRAM and it's properly detected

## License

See LICENSE file for details.
