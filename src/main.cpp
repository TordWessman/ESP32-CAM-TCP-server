#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"

// ============================================================
// MODE SELECTION - Choose ONE by uncommenting
// ============================================================
//#define USE_SERVER_MODE    // CameraTcpServer (relay pulls from ESP32)
#define USE_CLIENT_MODE // CameraRelayClient (ESP32 pushes to relay)
// ============================================================

#ifdef USE_SERVER_MODE
  #include "CameraTcpServer.h"
  #define PORT 1234
  #define FPS 30.0
  CameraTcpServer camera(PORT, FPS);
  #define MODE_NAME "Server Mode (Pull)"
#elif defined(USE_CLIENT_MODE)
  #include "CameraRelayClient.h"
  #define RELAY_HOST "oland.nejokey.se"  // Change to your relay server IP
  #define RELAY_PORT 4444
  #define FPS 10.0
  CameraRelayClient camera(RELAY_HOST, RELAY_PORT, FPS);
  #define MODE_NAME "Client Mode (Push)"
#else
  #error "Please define either USE_SERVER_MODE or USE_CLIENT_MODE"
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial monitor time to connect

  Serial.println("\n\n=================================");
  Serial.printf("ESP32-CAM Streaming: %s\n", MODE_NAME);
  Serial.println("=================================");

  // Connect to WiFi
  Serial.printf("\nConnecting to WiFi SSID: %s\n", SSID);
  WiFi.begin(SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\n✓ WiFi connected!");
  Serial.print("ESP32-CAM IP: ");
  Serial.println(WiFi.localIP());

  // Enable debug output
  camera.setDebug(true);

  // Initialize camera and network
  if (!camera.begin()) {
    Serial.println("ERROR: Camera initialization failed!");
    Serial.println("System halted.");
    while(1) delay(1000);
  }

  Serial.println("\n=================================");
  Serial.println("✓ System ready!");
  
#ifdef USE_SERVER_MODE
  Serial.printf("Listening on port %d\n", PORT);
  Serial.println("Waiting for relay server to connect...");
#else
  Serial.printf("Relay Server: %s:%d\n", RELAY_HOST, RELAY_PORT);
  Serial.println("Will connect to relay server...");
#endif
  
  Serial.printf("Target FPS: %.1f\n", FPS);
  Serial.println("=================================\n");
}

void loop() {
  // Run camera streaming (everything handled internally!)
#ifdef USE_SERVER_MODE
  CameraTcpServer::Status status = camera.run();
#else
  CameraRelayClient::Status status = camera.run();
#endif

  // Optional: Print stats every 10 seconds
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 10000) {
    Serial.printf("\n--- Statistics ---\n");
    Serial.printf("Frames sent: %u\n", camera.getFrameCount());
    Serial.printf("Bytes sent: %u\n", camera.getBytesSent());
    Serial.printf("Actual FPS: %.1f\n", camera.getActualFPS());
    
#ifdef USE_SERVER_MODE
    Serial.printf("Clients served: %u\n", camera.getClientCount());
    Serial.printf("Status: %s\n", CameraTcpServer::getStatusString(status));
#else
    Serial.printf("Connected: %s\n", camera.isConnected() ? "Yes" : "No");
    Serial.printf("Status: %s\n", CameraRelayClient::getStatusString(status));
#endif
    
    Serial.printf("------------------\n\n");
    lastStats = millis();
  }
}
