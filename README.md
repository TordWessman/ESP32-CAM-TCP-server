# ESP32-CAM TCP/UDP Streaming

ESP32-CAM JPEG video streaming with two operating modes. In **pull mode**, the ESP32 runs a TCP server and a relay fetches frames from it. In **push mode**, the ESP32 connects to a relay server and pushes frames over TCP or UDP. Built for AI Thinker ESP32-CAM with OV2640 camera using PlatformIO.

## Quick Start

1. Copy `include/secrets.h.example` to `include/secrets.h` and set your WiFi SSID and password
2. In `src/main.cpp`, uncomment `USE_SERVER_MODE` or `USE_CLIENT_MODE` (and set the relay host/port for client mode)
3. Build: `pio run`
4. Upload: connect FTDI adapter, connect GPIO0 to GND, press RESET, run `pio run --target upload`
5. Run: remove GPIO0 jumper, press RESET, open `pio device monitor`

## Modes

**Pull mode** -- `#define USE_SERVER_MODE` -- uses `CameraTcpServer`. ESP32 listens on port 1234. Relay server connects and pulls frames. ESP32 must be network-reachable.

**Push mode** -- `#define USE_CLIENT_MODE` -- uses `CameraRelayClient`. ESP32 connects to a relay and pushes frames. Works behind NAT. Supports TCP and UDP (UDP by default).

Switch modes by changing one `#define` in `src/main.cpp`.

## Relay Servers

Pull mode (relay connects to ESP32):
```bash
python3 relay_server.py --esp32-host 192.168.1.100 --esp32-port 1234 --client-port 8080
```

Push mode (ESP32 connects to relay):
```bash
python3 relay_server_receiver.py --sender-port 4444 --client-port 8080
```

Test client (capture frames from ESP32 or relay):
```bash
python3 test_client.py --host 192.168.1.100 --port 1234
python3 test_client.py --host relay.example.com --port 8080 --continuous --fps 5
```

## Project Structure

```
src/main.cpp                  Entry point with mode selection
include/CameraTcpServer.h     Camera + TCP server (pull mode)
include/CameraRelayClient.h   Camera + relay client (push mode)
include/TcpServer.h           Generic TCP server (no camera)
include/RelayClient.h         Generic TCP relay client (no camera)
include/NetworkClient.h       Abstract network interface
include/UDPNetworkClient.h    UDP with fragmentation
include/TCPNetworkClient.h    TCP wrapper
include/camera_config.h       OV2640 camera initialization
include/camera_pins.h         GPIO maps for 12+ board variants
relay_server.py               Pull-mode relay
relay_server_receiver.py      Push-mode relay
test_client.py                Frame capture test client
```

## Using as a Library

In another PlatformIO project's `platformio.ini`:

```ini
lib_deps =
    https://github.com/user/camera.tcp.git
build_flags =
    -DCAMERA_MODEL_AI_THINKER
```

## Build Commands

```bash
pio run                                  # Build
pio run --target upload                  # Upload (GPIO0 to GND first)
pio device monitor                       # Serial monitor
pio run --target upload --target monitor # Upload and monitor
pio run --target clean                   # Clean
```
