# ESP32-CAM TCP Client Mode

This alternative implementation makes the ESP32-CAM act as a TCP **client** that pushes frames to a relay server, instead of acting as a TCP server.

## Modes Comparison

### Server Mode (main.cpp - Current)
```
Internet Clients → Relay Server → ESP32-CAM (Server)
                                   Waits for connections
```

### Client Mode (main_client.cpp - This file)
```
ESP32-CAM (Client) → Relay Server → Internet Clients
Pushes frames         Receives       Multiple viewers
```

## When to Use Client Mode

✅ **Use Client Mode if:**
- ESP32-CAM is behind CGNAT (can't receive incoming connections)
- You have strict firewall rules blocking incoming connections
- Relay server is easier to reach than ESP32-CAM
- You want ESP32-CAM to actively push to the cloud

✅ **Use Server Mode if:**
- Relay server can reach ESP32-CAM (current setup works)
- You prefer the relay to control when frames are fetched
- Simpler setup (no ESP32 code changes needed)

## Setup

### Step 1: Configure Relay Server

The relay server needs to accept incoming connections from ESP32-CAM on one port, and serve clients on another.

**Important:** You need a DIFFERENT relay server for client mode!

Create `relay_server_receiver.py` on your VPS:

```python
#!/usr/bin/env python3
"""
Relay server that receives frames from ESP32-CAM and broadcasts to clients
"""
import socket
import threading
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')

class FrameRelay:
    def __init__(self):
        self.latest_frame = None
        self.clients = []
        
    def handle_esp32(self, conn, addr):
        logging.info(f"ESP32-CAM connected from {addr}")
        data = b''
        while True:
            chunk = conn.recv(4096)
            if not chunk:
                break
            data += chunk
        
        if data:
            self.latest_frame = data
            logging.info(f"Received frame: {len(data)} bytes")
            # Broadcast to all clients
            for client in self.clients[:]:
                try:
                    client.sendall(data)
                except:
                    self.clients.remove(client)
        conn.close()
    
    def handle_client(self, conn, addr):
        logging.info(f"Client connected from {addr}")
        self.clients.append(conn)
        if self.latest_frame:
            try:
                conn.sendall(self.latest_frame)
            except:
                pass
        # Keep alive
        while True:
            try:
                conn.recv(1)
            except:
                break
        self.clients.remove(conn)
        conn.close()
    
    def start(self):
        relay = FrameRelay()
        
        # Server for ESP32-CAM (sender)
        esp32_sock = socket.socket()
        esp32_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        esp32_sock.bind(('0.0.0.0', 1234))
        esp32_sock.listen(5)
        
        # Server for clients (viewers)
        client_sock = socket.socket()
        client_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        client_sock.bind(('0.0.0.0', 8080))
        client_sock.listen(10)
        
        logging.info("ESP32-CAM port: 1234")
        logging.info("Client port: 8080")
        
        def accept_esp32():
            while True:
                conn, addr = esp32_sock.accept()
                threading.Thread(target=relay.handle_esp32, args=(conn, addr), daemon=True).start()
        
        def accept_clients():
            while True:
                conn, addr = client_sock.accept()
                threading.Thread(target=relay.handle_client, args=(conn, addr), daemon=True).start()
        
        threading.Thread(target=accept_esp32, daemon=True).start()
        accept_clients()

if __name__ == '__main__':
    FrameRelay().start()
```

### Step 2: Configure ESP32-CAM

Edit `src/main_client.cpp`:

```cpp
#define RELAY_HOST "your-relay-server.com"  // Your VPS IP/hostname
#define RELAY_PORT 1234                      // Port where relay receives frames
#define FPS 10.0                             // Frames per second
```

### Step 3: Switch to Client Mode

Edit `platformio.ini` to use the client code:

```ini
[env:esp32cam]
platform = espressif32
board = esp32dev
framework = arduino

; Build configuration
build_src_filter = 
    +<*>
    -<main.cpp>          ; Exclude server mode
    +<main_client.cpp>   ; Include client mode

; ... rest of configuration
```

Or simply rename files:
```bash
cd src
mv main.cpp main_server.cpp
mv main_client.cpp main.cpp
```

### Step 4: Build and Upload

```bash
platformio run --target upload
```

### Step 5: Run Relay Server

On your VPS:
```bash
python3 relay_server_receiver.py
```

### Step 6: Test

Connect a client:
```bash
nc your-vps.com 8080 > frame.jpg
```

## Advantages of Client Mode

✅ ESP32-CAM controls frame rate (not relay)
✅ Works through NAT/firewalls (outbound connections)
✅ ESP32-CAM actively pushes to cloud
✅ Can reconnect automatically if connection drops

## Disadvantages of Client Mode

❌ Requires different relay server implementation
❌ ESP32-CAM uses more power (maintaining connection)
❌ More complex error handling needed
❌ Relay server must be always available

## Configuration Options

In `main_client.cpp`:

```cpp
#define RELAY_HOST "192.168.1.50"  // Relay server address
#define RELAY_PORT 1234             // Relay server port
#define FPS 10.0                    // Target frame rate
#define RETRY_DELAY 5000            // Reconnect delay (ms)
```

## Monitoring

Check ESP32-CAM serial output:
```
Frame #1: 45823 bytes sent, FPS: 0.50
Frame #2: 46102 bytes sent, FPS: 1.00
Frame #3: 45934 bytes sent, FPS: 1.50
```

## Switching Back to Server Mode

```bash
cd src
mv main.cpp main_client.cpp
mv main_server.cpp main.cpp
platformio run --target upload
```

Or update `platformio.ini` build filter.

## Troubleshooting

**Connection refused:**
- Make sure relay server is running
- Check RELAY_HOST and RELAY_PORT are correct
- Verify relay server accepts connections on port 1234

**Frames not appearing:**
- Check relay server logs
- Verify clients are connecting to port 8080
- Check firewall rules on VPS

**Low FPS:**
- Reduce FPS value in code
- Check network bandwidth
- Verify relay server can handle the data rate

## Current Status

- ✅ Client code created: `src/main_client.cpp`
- ⏳ Relay receiver server: Create `relay_server_receiver.py` on VPS
- ⏳ Switch build configuration
- ⏳ Test and deploy

Let me know if you want to switch to client mode!
