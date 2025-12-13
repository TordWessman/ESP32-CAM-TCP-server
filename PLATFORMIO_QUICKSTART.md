# Quick Start Guide - PlatformIO Migration

This Arduino project has been successfully converted to PlatformIO!

## What Changed?

### New Structure:
```
camera.tcp/
├── platformio.ini          # PlatformIO project configuration
├── src/
│   └── main.cpp           # Main code (was camera.tcp.ino)
├── include/
│   ├── camera_config.h    # Camera configuration
│   └── camera_pins.h      # Pin definitions
├── .vscode/
│   ├── tasks.json         # Build/upload tasks
│   └── extensions.json    # Recommended extensions
└── README_PLATFORMIO.md   # Updated documentation
```

### Key Files:
- **platformio.ini**: Contains ESP32-CAM board configuration, build flags, and dependencies
- **src/main.cpp**: Main application code (converted from .ino with `#include <Arduino.h>` added)
- **include/**: Header files for camera configuration

## Getting Started

### 1. Install PlatformIO

If you haven't already, install the PlatformIO extension in VS Code:
- Open VS Code Extensions (Ctrl+Shift+X)
- Search for "PlatformIO IDE"
- Click Install

### 2. Configure WiFi

Copy the secrets example file:
```bash
cp include/secrets.h.example include/secrets.h
```

Then edit `include/secrets.h` and update your credentials:
```cpp
#define SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"
```

**Note:** The `secrets.h` file is already in `.gitignore` to protect your credentials.

### 3. Build the Project

- Click the PlatformIO icon in the sidebar
- Select "Build" under "Project Tasks > esp32cam"

Or use the command line:
```bash
pio run
```

### 4. Upload to ESP32-CAM

**Important**: Put your ESP32-CAM in programming mode:
1. Connect GPIO0 to GND
2. Press the reset button (or power cycle)
3. Click Upload in PlatformIO or run:
   ```bash
   pio run --target upload
   ```
4. Disconnect GPIO0 from GND after upload
5. Press reset to run the program

### 5. Monitor Serial Output

View the serial output to see the IP address:
```bash
pio device monitor
```

Or click the "Monitor" task in PlatformIO.

## Advantages of PlatformIO

- **Better dependency management**: Automatically handles ESP32 camera libraries
- **Multiple board support**: Easy to switch between ESP32 variants
- **Built-in tools**: Serial monitor, debugger support, unit testing
- **Faster compilation**: Incremental builds are much faster
- **Professional workflow**: Industry-standard project structure

## Common Commands

```bash
# Build project
pio run

# Upload firmware
pio run --target upload

# Serial monitor
pio device monitor

# Clean build files
pio run --target clean

# Upload and monitor (combined)
pio run --target upload --target monitor
```

## Troubleshooting

### Build Errors
- Make sure PlatformIO is fully installed
- Try cleaning the project: `pio run --target clean`
- Delete the `.pio` folder and rebuild

### Upload Errors
- Ensure GPIO0 is connected to GND during programming
- Check USB connection and drivers (ESP32-CAM requires an FTDI adapter)
- Try lowering upload speed in `platformio.ini`: `upload_speed = 115200`

### Camera Init Failed
- Verify the camera model definition in `include/camera_config.h`
- Check that PSRAM is detected (should see "PSRAM found" in serial output)

## Next Steps

Check out `README_PLATFORMIO.md` for more detailed information about the project.

The original Arduino IDE files (`camera.tcp.ino`) are still in the root directory for reference, but you should now use the PlatformIO structure for development.

Happy coding! 🚀
