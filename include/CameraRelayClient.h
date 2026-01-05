/**
 * CameraRelayClient - Complete ESP32-CAM to Relay Streaming Solution
 * 
 * This module handles everything needed for camera streaming:
 * - Camera initialization and configuration
 * - Frame capture and encoding
 * - TCP connection to relay server
 * - Automatic reconnection and error handling
 * - FPS throttling
 * 
 * Usage:
 *   CameraRelayClient client("relay.example.com", 1234, 10.0);
 *   
 *   void setup() {
 *     WiFi.begin(ssid, password);
 *     while (WiFi.status() != WL_CONNECTED) delay(500);
 *     client.begin();
 *   }
 *   
 *   void loop() {
 *     CameraRelayClient::Status status = client.run();
 *     // That's it! Everything is handled internally
 *   }
 */

#ifndef CAMERA_RELAY_CLIENT_H
#define CAMERA_RELAY_CLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>
#include "esp_camera.h"

class CameraRelayClient {
public:
    /**
     * Status codes returned by run() method
     */
    enum Status {
        OK = 0,                    // Frame captured and sent successfully
        CAMERA_INIT_FAILED = 1,    // Camera initialization failed
        CAMERA_CAPTURE_FAILED = 2, // Frame capture failed
        NOT_CONNECTED = 3,         // Not connected to relay server
        SEND_FAILED = 4,           // Failed to send frame
        RECONNECTING = 5,          // Currently attempting to reconnect
        IDLE = 6                   // Waiting for next frame time (FPS throttling)
    };

    /**
     * Constructor
     * @param host Relay server hostname or IP address
     * @param port Relay server port
     * @param targetFPS Target frames per second (default: 10.0)
     */
    CameraRelayClient(const char* host, uint16_t port, float targetFPS = 10.0);
    
    /**
     * Initialize camera and client
     * Call this in setup() after WiFi is connected
     * @return true if initialization successful, false otherwise
     */
    bool begin();
    
    /**
     * Main loop - handles everything automatically
     * Call this in loop() - it's non-blocking!
     * @return Status code indicating current state
     */
    Status run();
    
    /**
     * Check if connected to relay server
     */
    bool isConnected() const { return _isConnected; }
    
    /**
     * Check if camera is initialized
     */
    bool isCameraReady() const { return _cameraInitialized; }
    
    /**
     * Set target FPS
     */
    void setTargetFPS(float fps);
    
    /**
     * Get target FPS
     */
    float getTargetFPS() const { return _targetFPS; }
    
    /**
     * Get statistics
     */
    uint32_t getFrameCount() const { return _frameCount; }
    uint32_t getBytesSent() const { return _bytesSent; }
    float getActualFPS() const;
    
    /**
     * Set retry delay in milliseconds (default: 5000)
     */
    void setRetryDelay(unsigned long delayMs) { _retryDelay = delayMs; }
    
    /**
     * Enable/disable debug output
     */
    void setDebug(bool enable) { _debug = enable; }
    
    /**
     * Get status description string
     * @param status Status code
     * @return Human-readable status description
     */
    static const char* getStatusString(Status status);
    
    /**
     * Disconnect from relay server
     */
    void disconnect();

private:
    // Configuration
    const char* _host;
    uint16_t _port;
    float _targetFPS;
    unsigned long _retryDelay;
    bool _debug;
    
    // Connection state
    WiFiClient _client;
    bool _isConnected;
    bool _cameraInitialized;
    unsigned long _lastConnectionAttempt;
    
    // Timing
    unsigned long _lastFrameTime;
    unsigned long _frameInterval;  // Calculated from FPS
    
    // Statistics
    uint32_t _frameCount;
    uint32_t _bytesSent;
    
    // FPS calculation
    unsigned long _fpsTimestamps[10];
    uint8_t _fpsIndex;
    
    // Internal methods
    bool initCamera();
    bool canSend() const;
    void attemptConnection();
    void updateFPS();
    void debugPrint(const char* message);
    void debugPrintf(const char* format, ...);
};

#endif // CAMERA_RELAY_CLIENT_H
