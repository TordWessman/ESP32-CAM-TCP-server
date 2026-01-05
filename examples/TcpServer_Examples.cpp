/**
 * TcpServer Module - Usage Examples
 * 
 * This file demonstrates various ways to use the TcpServer module
 * for streaming data over TCP with FPS control.
 */

#include "TcpServer.h"
#include "esp_camera.h"

// ============================================================================
// Example 1: Basic Usage with Camera
// ============================================================================
void example1_basic() {
  TcpServer server(1234, 10.0);  // Port 1234, 10 FPS
  
  void setup() {
    // ... WiFi and camera initialization ...
    server.begin();
  }
  
  void loop() {
    server.run();  // Handle connections
    
    if (server.hasClient() && server.canSend()) {
      camera_fb_t *fb = esp_camera_fb_get();
      server.sendData(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
  }
}

// ============================================================================
// Example 2: Dynamic FPS Control
// ============================================================================
void example2_dynamic_fps() {
  TcpServer server(1234, 10.0);
  
  void loop() {
    server.run();
    
    // Change FPS based on client count
    static uint32_t lastClientCount = 0;
    uint32_t currentClientCount = server.getClientCount();
    
    if (currentClientCount != lastClientCount) {
      if (server.hasClient()) {
        server.setTargetFPS(30.0);  // High FPS when client connected
      } else {
        server.setTargetFPS(5.0);   // Low FPS when idle
      }
      lastClientCount = currentClientCount;
    }
    
    if (server.hasClient() && server.canSend()) {
      // ... send data ...
    }
  }
}

// ============================================================================
// Example 3: Statistics Monitoring
// ============================================================================
void example3_statistics() {
  TcpServer server(1234, 10.0);
  
  void loop() {
    server.run();
    
    // Print stats every 5 seconds
    static unsigned long lastStats = 0;
    if (millis() - lastStats > 5000) {
      Serial.printf("Clients served: %u\n", server.getClientCount());
      Serial.printf("Frames sent: %u\n", server.getFrameCount());
      Serial.printf("Bytes sent: %u\n", server.getBytesSent());
      Serial.printf("Actual FPS: %.1f\n", server.getActualFPS());
      lastStats = millis();
    }
    
    if (server.hasClient() && server.canSend()) {
      // ... send data ...
    }
  }
}

// ============================================================================
// Example 4: Sensor Data Streaming (Non-Camera)
// ============================================================================
void example4_sensor_streaming() {
  TcpServer server(9999, 20.0);  // 20 Hz sensor data
  
  void loop() {
    server.run();
    
    if (server.hasClient() && server.canSend()) {
      // Read sensor data
      float temperature = readTemperature();
      float humidity = readHumidity();
      
      // Create data packet
      char buffer[64];
      int len = snprintf(buffer, sizeof(buffer), 
                        "TEMP:%.2f,HUMID:%.2f\n", 
                        temperature, humidity);
      
      server.sendData((uint8_t*)buffer, len);
    }
  }
}

// ============================================================================
// Example 5: Binary Data Streaming
// ============================================================================
void example5_binary_data() {
  TcpServer server(8888, 10.0);
  
  void loop() {
    server.run();
    
    if (server.hasClient() && server.canSend()) {
      // Prepare binary packet
      struct {
        uint32_t timestamp;
        float values[3];
        uint8_t status;
      } packet;
      
      packet.timestamp = millis();
      packet.values[0] = readSensor1();
      packet.values[1] = readSensor2();
      packet.values[2] = readSensor3();
      packet.status = getSystemStatus();
      
      server.sendData((uint8_t*)&packet, sizeof(packet));
    }
  }
}

// ============================================================================
// Example 6: Quality-Based FPS Adjustment
// ============================================================================
void example6_adaptive_quality() {
  TcpServer server(1234, 30.0);
  
  void loop() {
    server.run();
    
    if (server.hasClient() && server.canSend()) {
      // Adjust quality based on actual FPS
      float actualFPS = server.getActualFPS();
      
      if (actualFPS < server.getTargetFPS() * 0.8) {
        // Falling behind - reduce quality or FPS
        server.setTargetFPS(server.getTargetFPS() * 0.9);
        Serial.println("Reducing FPS due to performance");
      }
      
      camera_fb_t *fb = esp_camera_fb_get();
      server.sendData(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
  }
}

// ============================================================================
// Example 7: Manual Client Management
// ============================================================================
void example7_manual_disconnect() {
  TcpServer server(1234, 10.0);
  unsigned long sessionStart = 0;
  
  void loop() {
    server.run();
    
    // Track session start
    static bool wasConnected = false;
    if (server.hasClient() && !wasConnected) {
      sessionStart = millis();
      wasConnected = true;
    } else if (!server.hasClient()) {
      wasConnected = false;
    }
    
    // Disconnect after 5 minutes
    if (server.hasClient() && (millis() - sessionStart > 300000)) {
      Serial.println("Session timeout - disconnecting client");
      server.disconnectClient();
    }
    
    if (server.hasClient() && server.canSend()) {
      // ... send data ...
    }
  }
}

// ============================================================================
// Example 8: Multiple Data Sources
// ============================================================================
void example8_multiple_sources() {
  TcpServer cameraServer(1234, 30.0);
  TcpServer sensorServer(1235, 10.0);
  
  void loop() {
    // Handle both servers independently
    cameraServer.run();
    sensorServer.run();
    
    // Stream camera
    if (cameraServer.hasClient() && cameraServer.canSend()) {
      camera_fb_t *fb = esp_camera_fb_get();
      cameraServer.sendData(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
    
    // Stream sensor data
    if (sensorServer.hasClient() && sensorServer.canSend()) {
      char data[128];
      int len = createSensorPacket(data, sizeof(data));
      sensorServer.sendData((uint8_t*)data, len);
    }
  }
}
