/**
 * r2cam Example: Camera Relay Client (Push Mode)
 *
 * Demonstrates the CameraRelayClient SDK class which handles:
 *   - Camera initialization
 *   - Frame capture and JPEG encoding
 *   - TCP connection to a relay server with auto-reconnect
 *   - FPS throttling
 *   - Runtime statistics
 *
 * The ESP32-CAM pushes frames to a relay server. Viewers connect
 * to the relay to receive the stream. Use the Rust relay_server/
 * included in the r2cam project, or write your own.
 *
 * For UDP transport, pass a UDPNetworkClient to the constructor.
 *
 * Copy secrets.h.example to secrets.h and fill in your WiFi
 * credentials before building.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "CameraRelayClient.h"
#include "secrets.h"

// ---- Configuration --------------------------------------------------------

#define RELAY_HOST "192.168.1.100"  // Your relay server IP
#define RELAY_PORT 1234
#define TARGET_FPS 10.0

// ---- Global instances -----------------------------------------------------

CameraRelayClient camera(RELAY_HOST, RELAY_PORT, TARGET_FPS);

// ---- Arduino lifecycle ----------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("r2cam Relay Client example starting...");

    // Connect to WiFi
    WiFi.begin(SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWiFi connected - IP: %s\n", WiFi.localIP().toString().c_str());

    // Optional: enable debug output
    camera.setDebug(true);

    // Optional: tune retry/timeout behaviour
    camera.setRetryDelay(3000);   // reconnect after 3 s
    camera.setSendTimeout(500);   // drop frames that take > 500 ms to send

    // Initialize the camera hardware and connect to relay
    if (!camera.begin()) {
        Serial.println("Camera init failed!");
        return;
    }

    Serial.println("Camera ready, streaming to relay...");
}

void loop() {
    CameraRelayClient::Status status = camera.run();

    // Print statistics every 100 frames
    static uint32_t lastReport = 0;
    if (camera.getFrameCount() - lastReport >= 100) {
        lastReport = camera.getFrameCount();
        Serial.printf("[stats] frames=%u  bytes=%u  fps=%.1f  status=%s\n",
                      camera.getFrameCount(),
                      camera.getBytesSent(),
                      camera.getActualFPS(),
                      CameraRelayClient::getStatusString(status));
    }
}
