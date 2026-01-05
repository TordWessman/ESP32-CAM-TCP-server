# ESP32-CAM: Server vs Client Mode Comparison

## Quick Overview

You now have TWO ways to stream from your ESP32-CAM:

### Mode 1: SERVER MODE (Current - Recommended)
**Files:** `main.cpp` + `relay_server.py`

```
Relay Server → ESP32-CAM (Server :1234) ← Relay fetches frames
     ↓
Internet Clients (:8080)
```

**Relay server connects TO ESP32-CAM and fetches frames**

### Mode 2: CLIENT MODE (New - For Special Cases)
**Files:** `main_client.cpp` + `relay_server_receiver.py`

```
ESP32-CAM (Client) → Relay Server (:1234) ← ESP32 pushes frames
                          ↓
                  Internet Clients (:8080)
```

**ESP32-CAM connects TO relay server and pushes frames**

---

## Detailed Comparison

| Feature | Server Mode | Client Mode |
|---------|-------------|-------------|
| **ESP32 Code** | `main.cpp` | `main_client.cpp` |
| **Relay Server** | `relay_server.py` | `relay_server_receiver.py` |
| **ESP32 Role** | TCP Server | TCP Client |
| **Connection** | Relay connects to ESP32 | ESP32 connects to relay |
| **Frame Control** | Relay controls rate | ESP32 controls rate |
| **Setup Complexity** | Simple | Moderate |
| **Firewall/NAT** | Needs incoming access | Works anywhere (outbound) |
| **Best For** | Normal setups | CGNAT, strict firewalls |

---

## When to Use Each Mode

### Use SERVER MODE if:
✅ You can reach ESP32-CAM from the relay server  
✅ You're using SSH tunnel (recommended setup)  
✅ You want simpler configuration  
✅ **This is the recommended mode for most users**

### Use CLIENT MODE if:
✅ ESP32-CAM is behind CGNAT (can't receive connections)  
✅ You have strict firewall blocking incoming connections  
✅ You want ESP32-CAM to actively push to the cloud  
✅ You prefer ESP32-CAM to control frame rate

---

## How to Switch Between Modes

### Currently Active: SERVER MODE

To switch to **CLIENT MODE**:

```bash
cd /home/olga/workspace/Arduino/camera.tcp

# Option 1: Rename files
cd src
mv main.cpp main_server.cpp
mv main_client.cpp main.cpp

# Option 2: Or edit platformio.ini
# Add: build_src_filter = +<*> -<main.cpp> +<main_client.cpp>

# Edit main_client.cpp and set:
# #define RELAY_HOST "your-vps-ip"
# #define RELAY_PORT 1234

# Build and upload
platformio run --target upload
```

To switch back to **SERVER MODE**:

```bash
cd src
mv main.cpp main_client.cpp
mv main_server.cpp main.cpp
platformio run --target upload
```

---

## Relay Server Differences

### relay_server.py (for Server Mode)
```bash
# Relay connects TO ESP32-CAM
python3 relay_server.py \
    --esp32-host 192.168.1.100 \  # ESP32-CAM address
    --esp32-port 1234 \
    --client-port 8080
```

**Relay actively fetches from ESP32-CAM**

### relay_server_receiver.py (for Client Mode)
```bash
# Relay waits FOR ESP32-CAM to connect
python3 relay_server_receiver.py \
    --sender-port 1234 \   # ESP32-CAM connects here
    --client-port 8080      # Clients connect here
```

**Relay passively receives from ESP32-CAM**

---

## Testing Setup

### Test Server Mode (Current)

1. **Local test:**
```bash
# Terminal 1: Start relay
python3 relay_server.py --esp32-host 192.168.1.100 --esp32-port 1234 --client-port 8080

# Terminal 2: Get frame
./test_client.py --host localhost --port 8080
```

2. **With SSH tunnel:**
```bash
# Terminal 1: Create tunnel (local machine)
ssh -R 1234:192.168.1.100:1234 user@vps.com

# Terminal 2: Run relay (on VPS)
python3 relay_server.py --esp32-host localhost --esp32-port 1234 --client-port 8080

# Terminal 3: Test (from anywhere)
nc vps.com 8080 > frame.jpg
```

### Test Client Mode (New)

1. **Configure ESP32:**
```cpp
// In main_client.cpp
#define RELAY_HOST "192.168.1.50"  // Your machine IP
#define RELAY_PORT 1234
```

2. **Switch code:**
```bash
cd src
mv main.cpp main_server.cpp
mv main_client.cpp main.cpp
```

3. **Upload:**
```bash
platformio run --target upload
```

4. **Start relay receiver:**
```bash
# Terminal 1: Start receiver relay
python3 relay_server_receiver.py --sender-port 1234 --client-port 8080

# Terminal 2: Monitor ESP32-CAM serial output
# Should see: "Connecting to relay server..." then "Connected"

# Terminal 3: Get frames
./test_client.py --host localhost --port 8080
```

---

## Recommendations

### For Most Users:
👉 **Stay with SERVER MODE** (main.cpp + relay_server.py)
- Simpler setup
- Relay controls everything
- Works great with SSH tunnel

### For Special Cases:
👉 **Use CLIENT MODE** (main_client.cpp + relay_server_receiver.py)
- ESP32 behind CGNAT
- Can't set up port forwarding
- Want ESP32 to push to cloud

---

## Current Status

✅ Server Mode: **ACTIVE** (main.cpp)  
✅ Client Mode: **READY** (main_client.cpp - not active)  
✅ Relay Server (Pull): **CREATED** (relay_server.py)  
✅ Relay Server (Push): **CREATED** (relay_server_receiver.py)  

**You can switch anytime by renaming the main.cpp files!**

---

## Summary

You asked for a TCP client module, and I've created it! However, **the current server mode with relay_server.py already works perfectly** for your use case. 

The client mode (`main_client.cpp` + `relay_server_receiver.py`) is there if you need it, but **I recommend sticking with server mode** unless you have a specific reason to switch.

**Bottom line:** You now have both options available! 🚀
