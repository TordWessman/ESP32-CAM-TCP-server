#!/bin/bash
# SSH Reverse Tunnel Setup for ESP32-CAM
# This creates a permanent tunnel from your local ESP32-CAM to a remote server

# ============================================
# CONFIGURATION - EDIT THESE VALUES
# ============================================

# Your remote server details
REMOTE_HOST="your-server.com"
REMOTE_USER="your-username"
REMOTE_PORT="8080"  # Port on remote server where ESP32-CAM will be accessible

# Local ESP32-CAM details
LOCAL_HOST="192.168.1.100"  # Replace with your ESP32-CAM's IP
LOCAL_PORT="1234"

# SSH key path (for passwordless login)
SSH_KEY="$HOME/.ssh/id_rsa"

# ============================================
# INSTALLATION
# ============================================

echo "==================================="
echo "ESP32-CAM SSH Tunnel Setup"
echo "==================================="
echo ""

# Check if autossh is installed
if ! command -v autossh &> /dev/null; then
    echo "Installing autossh..."
    sudo apt-get update
    sudo apt-get install -y autossh
fi

# Generate SSH key if it doesn't exist
if [ ! -f "$SSH_KEY" ]; then
    echo "Generating SSH key..."
    ssh-keygen -t rsa -b 4096 -f "$SSH_KEY" -N ""
    echo ""
    echo "Copy this public key to your remote server:"
    echo "ssh-copy-id -i $SSH_KEY.pub $REMOTE_USER@$REMOTE_HOST"
    echo ""
    read -p "Press ENTER after you've copied the key to continue..."
fi

# Test connection
echo "Testing connection to $REMOTE_HOST..."
ssh -i "$SSH_KEY" -o "BatchMode=yes" -o "ConnectTimeout=5" "$REMOTE_USER@$REMOTE_HOST" "echo 'Connection successful!'" || {
    echo "ERROR: Cannot connect to remote server!"
    echo "Make sure you've copied your SSH key:"
    echo "ssh-copy-id -i $SSH_KEY.pub $REMOTE_USER@$REMOTE_HOST"
    exit 1
}

# ============================================
# CREATE SYSTEMD SERVICE
# ============================================

echo "Creating systemd service..."

SERVICE_FILE="/etc/systemd/system/esp32cam-tunnel.service"

sudo tee "$SERVICE_FILE" > /dev/null <<EOF
[Unit]
Description=ESP32-CAM SSH Reverse Tunnel
After=network.target

[Service]
Type=simple
User=$USER
Restart=always
RestartSec=10
ExecStart=/usr/bin/autossh -M 0 \\
    -o "ServerAliveInterval=30" \\
    -o "ServerAliveCountMax=3" \\
    -o "ExitOnForwardFailure=yes" \\
    -o "StrictHostKeyChecking=no" \\
    -i $SSH_KEY \\
    -N \\
    -R $REMOTE_PORT:$LOCAL_HOST:$LOCAL_PORT \\
    $REMOTE_USER@$REMOTE_HOST

[Install]
WantedBy=multi-user.target
EOF

# ============================================
# START SERVICE
# ============================================

echo "Enabling and starting service..."
sudo systemctl daemon-reload
sudo systemctl enable esp32cam-tunnel.service
sudo systemctl start esp32cam-tunnel.service

# ============================================
# STATUS
# ============================================

echo ""
echo "==================================="
echo "Setup Complete!"
echo "==================================="
echo ""
echo "Your ESP32-CAM is now accessible at:"
echo "  $REMOTE_HOST:$REMOTE_PORT"
echo ""
echo "Test it with:"
echo "  nc $REMOTE_HOST $REMOTE_PORT > stream.jpg"
echo ""
echo "Service status:"
sudo systemctl status esp32cam-tunnel.service --no-pager
echo ""
echo "Useful commands:"
echo "  sudo systemctl status esp32cam-tunnel   # Check status"
echo "  sudo systemctl restart esp32cam-tunnel  # Restart tunnel"
echo "  sudo systemctl stop esp32cam-tunnel     # Stop tunnel"
echo "  sudo journalctl -u esp32cam-tunnel -f   # View logs"
echo ""
