/**
 * RelayClient.h
 * 
 * TCP client for streaming data to a relay server
 * Non-blocking implementation suitable for ESP32
 * 
 * Usage:
 *   RelayClient client("192.168.1.50", 1234);
 *   client.begin();
 *   
 *   // In loop:
 *   client.run();
 *   if (client.isConnected() && client.canSend()) {
 *     client.sendData(buffer, length);
 *   }
 */

#ifndef RELAY_CLIENT_H
#define RELAY_CLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>

class RelayClient {
public:
    /**
     * Constructor
     * @param host Relay server hostname or IP address
     * @param port Relay server port
     * @param targetFPS Target frames per second (0 = no limit)
     */
    RelayClient(const char* host, uint16_t port, float targetFPS = 0);
    
    /**
     * Initialize the client
     */
    void begin();
    
    /**
     * Non-blocking run method - call this in loop()
     * Handles connection state, reconnection, and timing
     */
    void run();
    
    /**
     * Send data to relay server
     * @param data Pointer to data buffer
     * @param length Length of data in bytes
     * @return true if data was sent successfully
     */
    bool sendData(const uint8_t* data, size_t length);
    
    /**
     * Check if connected to relay server
     */
    bool isConnected() const;
    
    /**
     * Check if ready to send next frame (based on FPS throttling)
     */
    bool canSend() const;
    
    /**
     * Disconnect from server
     */
    void disconnect();
    
    /**
     * Set target FPS (0 = no limit)
     */
    void setTargetFPS(float fps);
    
    /**
     * Get statistics
     */
    unsigned long getFrameCount() const { return _frameCount; }
    unsigned long getTotalBytesSent() const { return _totalBytesSent; }
    float getActualFPS() const;
    
    /**
     * Set retry delay in milliseconds (default: 5000)
     */
    void setRetryDelay(unsigned long delayMs) { _retryDelay = delayMs; }
    
    /**
     * Enable/disable debug output
     */
    void setDebug(bool enable) { _debug = enable; }

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
    unsigned long _lastConnectionAttempt;
    
    // Timing
    unsigned long _lastSendTime;
    unsigned long _minSendInterval;  // Calculated from FPS
    
    // Statistics
    unsigned long _frameCount;
    unsigned long _totalBytesSent;
    unsigned long _startTime;
    
    // Internal methods
    void attemptConnection();
    void updateFPSInterval();
    void debugPrint(const char* message);
    void debugPrintf(const char* format, ...);
};

#endif // RELAY_CLIENT_H
