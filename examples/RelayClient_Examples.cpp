/**
 * RelayClient Module - Usage Examples
 * 
 * This demonstrates how to use the RelayClient module
 * independently of camera code.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "RelayClient.h"

// Example 1: Basic Usage
// =====================================================

void example1_basic() {
    // Create client instance
    RelayClient client("192.168.1.50", 1234);
    
    // Initialize
    client.begin();
    client.setDebug(true);
    
    // In your loop:
    // client.run();  // Call every loop iteration
    // 
    // if (client.isConnected()) {
    //     uint8_t data[] = {0x01, 0x02, 0x03};
    //     client.sendData(data, sizeof(data));
    // }
}

// Example 2: With FPS Control
// =====================================================

void example2_fps_control() {
    // Create client with 10 FPS limit
    RelayClient client("192.168.1.50", 1234, 10.0);
    
    client.begin();
    
    // In loop:
    // client.run();
    // 
    // // Only send when ready (respects FPS limit)
    // if (client.canSend()) {
    //     uint8_t data[] = {...};
    //     client.sendData(data, len);
    // }
}

// Example 3: Camera Integration (Clean Separation)
// =====================================================

#include "esp_camera.h"

RelayClient cameraRelay("relay.example.com", 1234, 15.0);

void setup() {
    Serial.begin(115200);
    
    // Setup WiFi
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
    
    // Setup camera (your existing code)
    // camera_config_t config = {...};
    // esp_camera_init(&config);
    
    // Setup relay client
    cameraRelay.setDebug(true);
    cameraRelay.setRetryDelay(3000);  // 3 second retry
    cameraRelay.begin();
}

void loop() {
    // Non-blocking relay client update
    cameraRelay.run();
    
    // Send frame when ready
    if (cameraRelay.isConnected() && cameraRelay.canSend()) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            // Send to relay
            cameraRelay.sendData(fb->buf, fb->len);
            
            // Free buffer
            esp_camera_fb_return(fb);
        }
    }
    
    // Other loop code...
}

// Example 4: Generic Data Streaming
// =====================================================

class SensorDataStreamer {
private:
    RelayClient relay;
    
public:
    SensorDataStreamer(const char* host, uint16_t port) 
        : relay(host, port, 1.0)  // 1 sample per second
    {
    }
    
    void begin() {
        relay.begin();
        relay.setDebug(false);  // Quiet mode
    }
    
    void update() {
        relay.run();  // Must call every loop
    }
    
    void sendSensorData(float temperature, float humidity) {
        if (!relay.canSend()) {
            return;  // Throttled by FPS limit
        }
        
        // Pack data
        struct {
            float temp;
            float humid;
            uint32_t timestamp;
        } packet;
        
        packet.temp = temperature;
        packet.humid = humidity;
        packet.timestamp = millis();
        
        // Send to relay
        relay.sendData((uint8_t*)&packet, sizeof(packet));
    }
    
    void printStats() {
        Serial.printf("Frames sent: %lu, FPS: %.2f\n",
                     relay.getFrameCount(),
                     relay.getActualFPS());
    }
};

// Example usage:
// SensorDataStreamer sensor("relay.local", 1234);
// sensor.begin();
// 
// void loop() {
//     sensor.update();
//     sensor.sendSensorData(23.5, 45.2);
//     sensor.printStats();
// }

// Example 5: Dynamic FPS Adjustment
// =====================================================

void example5_dynamic_fps() {
    RelayClient client("192.168.1.50", 1234);
    client.begin();
    
    // In loop, adjust FPS based on conditions:
    // 
    // if (batteryLow) {
    //     client.setTargetFPS(5.0);   // Slow down
    // } else {
    //     client.setTargetFPS(30.0);  // Speed up
    // }
    //
    // // Or disable FPS limit entirely:
    // client.setTargetFPS(0);  // Send as fast as possible
}

// Example 6: Error Handling
// =====================================================

void example6_error_handling() {
    RelayClient client("192.168.1.50", 1234, 10.0);
    client.begin();
    
    // In loop:
    // client.run();
    //
    // if (client.isConnected()) {
    //     uint8_t data[] = {...};
    //     bool success = client.sendData(data, len);
    //     
    //     if (!success) {
    //         Serial.println("Send failed - will auto-reconnect");
    //         // Client handles reconnection automatically
    //     }
    // } else {
    //     Serial.println("Not connected - waiting for reconnect...");
    //     // Client is automatically trying to reconnect
    // }
}

// Example 7: Statistics Monitoring
// =====================================================

RelayClient statsClient("192.168.1.50", 1234, 10.0);

void printStatistics() {
    Serial.println("=== Relay Client Statistics ===");
    Serial.printf("Frames sent: %lu\n", statsClient.getFrameCount());
    Serial.printf("Bytes sent: %lu\n", statsClient.getTotalBytesSent());
    Serial.printf("Actual FPS: %.2f\n", statsClient.getActualFPS());
    Serial.printf("Connected: %s\n", statsClient.isConnected() ? "Yes" : "No");
}

// Call printStatistics() periodically to monitor performance

// Example 8: Multiple Relay Clients
// =====================================================

void example8_multiple_clients() {
    // Send to different relay servers
    RelayClient relay1("relay1.example.com", 1234, 15.0);
    RelayClient relay2("relay2.example.com", 5678, 10.0);
    
    relay1.begin();
    relay2.begin();
    
    // In loop:
    // relay1.run();
    // relay2.run();
    //
    // if (relay1.canSend()) {
    //     // Send high-quality stream to relay1
    //     relay1.sendData(highQualityData, len1);
    // }
    //
    // if (relay2.canSend()) {
    //     // Send low-quality stream to relay2
    //     relay2.sendData(lowQualityData, len2);
    // }
}

// =====================================================
// Key Features of RelayClient Module
// =====================================================

/*
 * ✅ Non-blocking - Safe to call run() in loop()
 * ✅ Auto-reconnect - Handles connection failures
 * ✅ FPS throttling - Built-in rate limiting
 * ✅ Independent - No camera dependencies
 * ✅ Statistics - Track frames, bytes, FPS
 * ✅ Configurable - Retry delay, debug mode
 * ✅ Simple API - Easy to integrate
 * 
 * Perfect for:
 * - Camera streaming
 * - Sensor data logging
 * - Generic TCP client needs
 * - Real-time data transmission
 */
