/**
 * CameraTcpServer - Complete ESP32-CAM TCP Server Solution
 * 
 * This module handles everything needed for camera serving:
 * - Camera initialization and configuration
 * - Frame capture and encoding
 * - TCP server for accepting connections
 * - Automatic client management
 * - FPS throttling
 * 
 * Usage:
 *   CameraTcpServer server(1234, 30.0);
 *   
 *   void setup() {
 *     WiFi.begin(ssid, password);
 *     while (WiFi.status() != WL_CONNECTED) delay(500);
 *     server.begin();
 *   }
 *   
 *   void loop() {
 *     CameraTcpServer::Status status = server.run();
 *     // That's it! Everything is handled internally
 *   }
 */

#ifndef CAMERA_TCP_SERVER_H
#define CAMERA_TCP_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"

class CameraTcpServer {
public:
    /**
     * Status codes returned by run() method
     */
    enum Status {
        OK = 0,                    // Frame captured and sent successfully
        CAMERA_INIT_FAILED = 1,    // Camera initialization failed
        CAMERA_CAPTURE_FAILED = 2, // Frame capture failed
        NO_CLIENT = 3,             // No client connected
        SEND_FAILED = 4,           // Failed to send frame
        IDLE = 5                   // Waiting for next frame time (FPS throttling)
    };

    /**
     * Constructor
     * @param port TCP port to listen on
     * @param targetFPS Target frames per second (default: 10.0)
     */
    CameraTcpServer(uint16_t port, float targetFPS = 10.0);
    
    /**
     * Initialize camera and server
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
     * Check if a client is currently connected
     */
    bool hasClient() const { return _hasClient; }
    
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
    uint32_t getClientCount() const { return _clientCount; }
    uint32_t getFrameCount() const { return _frameCount; }
    uint32_t getBytesSent() const { return _bytesSent; }
    float getActualFPS() const;
    
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
     * Disconnect current client
     */
    void disconnectClient();

private:
    // Configuration
    uint16_t _port;
    float _targetFPS;
    bool _debug;
    
    // Network
    WiFiServer _server;
    WiFiClient _client;
    bool _hasClient;
    bool _cameraInitialized;
    
    // Timing
    unsigned long _lastFrameTime;
    unsigned long _frameInterval;  // Calculated from FPS
    
    // Statistics
    uint32_t _clientCount;
    uint32_t _frameCount;
    uint32_t _bytesSent;
    
    // FPS calculation
    unsigned long _fpsTimestamps[10];
    uint8_t _fpsIndex;
    
    // Internal methods
    bool initCamera();
    bool canSend() const;
    void updateFPS();
    void debugPrint(const char* message);
    void debugPrintf(const char* format, ...);
};

#endif // CAMERA_TCP_SERVER_H
