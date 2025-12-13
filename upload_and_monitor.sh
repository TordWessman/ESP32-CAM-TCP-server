#!/bin/bash
# ESP32-CAM Upload and Monitor Helper Script
# This script guides you through the upload and monitor process

set -e

echo "======================================"
echo "ESP32-CAM Upload and Monitor Helper"
echo "======================================"
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}STEP 1: Prepare for Upload${NC}"
echo "1. Connect GPIO0 to GND on your ESP32-CAM"
echo "2. Press the RESET button on the ESP32-CAM"
echo ""
read -p "Press ENTER when ready to upload..."

echo ""
echo -e "${GREEN}Uploading firmware...${NC}"
~/.platformio/penv/bin/platformio run --target upload --environment esp32cam

echo ""
echo -e "${RED}CRITICAL: Now disconnect GPIO0 from GND!${NC}"
echo -e "${RED}Then press the RESET button on the ESP32-CAM${NC}"
echo ""
read -p "Press ENTER when GPIO0 is disconnected and you've pressed RESET..."

echo ""
echo -e "${GREEN}Starting serial monitor...${NC}"
echo "Press Ctrl+C to exit the monitor"
echo ""
sleep 2

~/.platformio/penv/bin/platformio device monitor --port /dev/ttyUSB0 --baud 115200
