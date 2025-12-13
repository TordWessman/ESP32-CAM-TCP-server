# ESP32-CAM Upload Guide

## The Problem
ESP32-CAM modules don't have built-in USB-to-serial converters or auto-reset circuits like most ESP32 dev boards. This means you need to manually put the board into programming mode.

## Hardware Requirements

You need an **FTDI adapter** or similar USB-to-serial converter to program the ESP32-CAM:

### Wiring Diagram
```
FTDI/USB-Serial    ESP32-CAM
---------------------------------
5V       -------->  5V
GND      -------->  GND
TX       -------->  U0R (RX)
RX       -------->  U0T (TX)

For Programming Mode:
GPIO0    -------->  GND (via jumper wire)
```

⚠️ **Important**: Some ESP32-CAM boards work better with 5V, while others prefer 3.3V. If you have stability issues, try switching the voltage.

## Upload Procedure

### Step-by-Step Instructions:

1. **Connect the wiring** as shown above (without GPIO0 to GND yet)

2. **Put board in programming mode:**
   - Connect a jumper wire from **GPIO0 to GND**
   - Press the **RESET button** (or disconnect and reconnect power)
   - The board is now in bootloader mode

3. **Upload the firmware:**
   - In VS Code, click the PlatformIO Upload button
   - Or run: `~/.platformio/penv/bin/platformio run --target upload`
   - Wait for "Connecting........" message
   - Upload should start and complete

4. **Run the program:**
   - **Remove the GPIO0 to GND jumper**
   - Press the **RESET button** again
   - The program should now run

5. **Monitor serial output:**
   - Click the Serial Monitor button in PlatformIO
   - Or run: `~/.platformio/penv/bin/platformio device monitor`
   - You should see WiFi connection messages and the server IP address

## Common Issues & Solutions

### Issue 1: "Unable to verify flash chip connection"
**Symptoms:** Error message about invalid packet header (0xE0, 0xC0, etc.)

**Solutions:**
- ✅ Make sure GPIO0 is connected to GND before pressing reset
- ✅ Press the RESET button after connecting GPIO0 to GND
- ✅ Check all wiring connections (especially RX/TX - they might need to be crossed)
- ✅ Try a slower upload speed (already set to 115200 in platformio.ini)
- ✅ Try powering with 5V instead of 3.3V (or vice versa)
- ✅ Use a shorter USB cable or different USB port

### Issue 2: "Timed out waiting for packet header"
**Solutions:**
- ✅ Verify RX and TX are connected correctly (TX->RX, RX->TX)
- ✅ Check that the FTDI adapter is recognized by your computer
- ✅ Try a different USB port or cable
- ✅ Verify the baud rate matches (115200)

### Issue 3: Upload succeeds but program doesn't run
**Solutions:**
- ✅ Make sure you **removed the GPIO0 to GND jumper** after upload
- ✅ Press the RESET button to restart the board
- ✅ Check serial monitor for error messages

### Issue 4: Board keeps resetting or behaves erratically
**Solutions:**
- ✅ Use external 5V power supply (not just USB)
- ✅ Add a 100µF capacitor between 5V and GND
- ✅ The camera draws significant current, USB might not provide enough

## Quick Reference: Upload Checklist

```
☐ 1. Wiring connected (5V, GND, TX->RX, RX->TX)
☐ 2. GPIO0 connected to GND
☐ 3. Press RESET button
☐ 4. Start upload in PlatformIO
☐ 5. Wait for upload to complete (100%)
☐ 6. Remove GPIO0 to GND jumper
☐ 7. Press RESET button again
☐ 8. Open Serial Monitor to verify
```

## Visual Pin Reference

```
ESP32-CAM Pins (top view):
                    ___________
                   |           |
        GND  O     |   ESP32   |     O  5V
       GPIO1 O     |    CAM    |     O  GND
       GPIO3 O     |           |     O  GPIO2
        SD2  O     |___________|     O  GPIO4
        SD3  O                       O  GPIO12
        CMD  O                       O  GPIO13
        CLK  O      [ Camera ]       O  GPIO14
        SD0  O                       O  GPIO15
        SD1  O                       O  GPIO0  <-- Connect to GND for programming
                   |___________|
```

## PlatformIO Commands

```bash
# Build only
~/.platformio/penv/bin/platformio run

# Upload (with GPIO0->GND first!)
~/.platformio/penv/bin/platformio run --target upload

# Serial monitor (after removing GPIO0->GND)
~/.platformio/penv/bin/platformio device monitor

# Combined upload and monitor
~/.platformio/penv/bin/platformio run --target upload --target monitor
# Note: For combined, remove GPIO0->GND jumper as soon as upload completes
```

## Troubleshooting Serial Connection

Check if your FTDI adapter is detected:
```bash
# Linux
ls /dev/ttyUSB*
# or
ls /dev/ttyACM*

# Check permissions (Linux)
sudo usermod -a -G dialout $USER
# Then log out and back in
```

Add to `platformio.ini` if needed:
```ini
upload_port = /dev/ttyUSB0
monitor_port = /dev/ttyUSB0
```

## Alternative: Using Arduino IDE

If you prefer to use Arduino IDE temporarily for uploads:
1. Open `camera.tcp.ino` in Arduino IDE
2. Select **Tools > Board > ESP32 Arduino > AI Thinker ESP32-CAM**
3. Select your serial port under **Tools > Port**
4. Follow the same GPIO0->GND procedure
5. Click Upload

## Success Indicators

When everything works correctly, you should see:
1. **During upload**: Progress bar reaching 100%
2. **After reset**: Serial output showing:
   - "Connecting to WiFi..."
   - "Server started on port 1234"
   - "Address: 192.168.x.x" (your ESP32-CAM's IP)

---

**Remember:** The GPIO0->GND jumper is only needed during upload, not during normal operation!
