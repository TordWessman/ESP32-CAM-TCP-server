/**
 * r2cam Example: Camera TCP Server (Pull Mode)
 *
 * Demonstrates the CameraTcpServer SDK class which handles:
 *   - Camera initialization
 *   - TCP server that accepts a client connection
 *   - Frame capture and streaming to connected client
 *   - FPS throttling and automatic client management
 *
 * In pull mode the ESP32-CAM acts as a server. A client (e.g.
 * the included test_client.py or any TCP socket) connects and
 * receives JPEG frames.
 *
 * Copy secrets.h.example to secrets.h and fill in your WiFi
 * credentials before building.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "CameraTcpServer.h"
#include "secrets.h"

// ---- Configuration --------------------------------------------------------

#define SERVER_PORT 1234
#define TARGET_FPS  15.0

// ---- Global instances -----------------------------------------------------

CameraTcpServer camServer(SERVER_PORT, TARGET_FPS);

// ---- Arduino lifecycle ----------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("r2cam TCP Server example starting...");

    // Connect to WiFi
    WiFi.begin(SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWiFi connected - IP: %s\n", WiFi.localIP().toString().c_str());

    // Optional: enable debug logging
    camServer.setDebug(true);

    // Initialize camera and start listening
    if (!camServer.begin()) {
        Serial.println("Camera/server init failed!");
        return;
    }

    Serial.printf("Camera TCP server listening on port %d\n", SERVER_PORT);
    Serial.println("Connect with: python test_client.py <ESP_IP> 1234");
}

void loop() {
    CameraTcpServer::Status status = camServer.run();

    // Report when a client connects or disconnects
    static bool hadClient = false;
    if (camServer.hasClient() && !hadClient) {
        Serial.println("[server] Client connected");
    }
    if (!camServer.hasClient() && hadClient) {
        Serial.printf("[server] Client disconnected (sent %u frames, %u bytes)\n",
                      camServer.getFrameCount(), camServer.getBytesSent());
    }
    hadClient = camServer.hasClient();
}
