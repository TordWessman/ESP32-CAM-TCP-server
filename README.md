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

## Relay Server

The relay server is written in Rust and located in `relay_server/`. It receives frames pushed by the ESP32-CAM (via TCP or UDP) and broadcasts them to connected clients.

```
ESP32-CAM -> [TCP :4444 / UDP :8081] -> Relay Server -> [TCP :8080] -> Clients
```

### Building

```bash
cd relay_server
cargo build --release
```

### Running

```bash
# Default ports (TCP 4444, UDP 8081, clients 8080)
./target/release/relay_server_receiver

# Custom ports
./target/release/relay_server_receiver --sender-port 4444 --udp-port 8081 --client-port 8080

# With debug logging
./target/release/relay_server_receiver --debug
```

### Options

| Flag | Default | Description |
|------|---------|-------------|
| `--sender-host` | `0.0.0.0` | Interface to listen for ESP32-CAM TCP |
| `--sender-port` | `4444` | Port for ESP32-CAM TCP connections |
| `--udp-port` | `8081` | Port for ESP32-CAM UDP packets (0 to disable) |
| `--client-host` | `0.0.0.0` | Interface to listen for viewer clients |
| `--client-port` | `8080` | Port for clients to connect to |
| `--debug` | off | Enable debug logging |

## Project Structure

```
src/main.cpp                  Entry point with mode selection
src/CameraRelayClient.cpp     Camera + relay client (push mode)
src/CameraTcpServer.cpp       Camera + TCP server (pull mode)
src/RelayClient.cpp           Generic TCP relay client (no camera)
src/TcpServer.cpp             Generic TCP server (no camera)
include/CameraRelayClient.h   Camera + relay client header
include/CameraTcpServer.h     Camera + TCP server header
include/RelayClient.h         Generic relay client header
include/TcpServer.h           Generic TCP server header
include/NetworkClient.h       Abstract network interface
include/UDPNetworkClient.h    UDP with fragmentation
include/TCPNetworkClient.h    TCP wrapper
include/camera_config.h       OV2640 camera initialization
include/camera_pins.h         GPIO maps for 12+ board variants
relay_server/                 Rust relay server (push mode)
examples/tcp_server/          Example TCP server usage
```

## Using as a Library

In another PlatformIO project's `platformio.ini`:

```ini
lib_deps =
    https://github.com/user/r2cam.git
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
