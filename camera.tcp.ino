#include <WiFi.h>
#include "esp_camera.h"
#include "camera_config.h"

// WiFi name
#define SSID ""
// WiFi password
#define WIFI_PASSWORD ""
// TCP port
#define PORT 1234
// Frames Per Second
#define FPS 30.0

WiFiServer server(PORT);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Initialize camera
  bool cameraConfigured = createCameraConfiguration();

  if (!cameraConfigured) {
    return;
  }

  // Connect to WiFi
  WiFi.begin(SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Start TCP server
  server.begin();
  Serial.printf("Server started on port %d\n", PORT);
  Serial.print("Address: "); Serial.println(WiFi.localIP());
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.print("New client connected: ");
    Serial.println(client.remoteIP());
    while (client.connected()) {
      // Capture image
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Failed to capture image");
        break;
      }

      // Send image data over TCP
      if (client.write(fb->buf, fb->len) != fb->len) {
        Serial.println("Error sending image");
      }

      // Free image buffer
      esp_camera_fb_return(fb);
      
      // Delay between frames (adjust as needed)
      delay(1000 * (int)(1.0f / FPS)); // 30 frames per second
    }

    // Close client connection
    client.stop();
    Serial.println("Client disconnected");
  }
}