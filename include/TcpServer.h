/**
 * TcpServer - Modular TCP Server for ESP32
 * 
 * A reusable, camera-independent TCP server module that serves data to connecting clients.
 * Designed for continuous streaming with FPS control and automatic client management.
 * 
 * Features:
 * - Non-blocking client handling
 * - Built-in FPS throttling
 * - Automatic client disconnection on errors
 * - Statistics tracking (clients served, frames sent, bytes sent)
 * - Zero dependencies on camera code
 * 
 * Usage:
 *   TcpServer server(1234, 10.0);  // Port 1234, 10 FPS
 *   server.begin();
 *   
 *   void loop() {
 *     server.run();
 *     if (server.canSend()) {
 *       server.sendData(buffer, size);
 *     }
 *   }
 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <WiFi.h>

class TcpServer {
public:
  /**
   * Constructor
   * @param port TCP port to listen on
   * @param targetFPS Target frames per second for data transmission
   */
  TcpServer(uint16_t port, float targetFPS = 10.0);
  
  /**
   * Initialize and start the TCP server
   * Must be called in setup() after WiFi is connected
   */
  void begin();
  
  /**
   * Main server loop - handles client connections
   * Call this in loop() - it's non-blocking!
   */
  void run();
  
  /**
   * Send data to connected client
   * @param data Pointer to data buffer
   * @param length Size of data in bytes
   * @return true if data was sent successfully, false otherwise
   */
  bool sendData(const uint8_t* data, size_t length);
  
  /**
   * Check if we're ready to send the next frame (FPS throttling)
   * @return true if enough time has passed since last send
   */
  bool canSend();
  
  /**
   * Check if a client is currently connected
   * @return true if client is connected
   */
  bool hasClient();
  
  /**
   * Set new target FPS
   * @param fps New frames per second target
   */
  void setTargetFPS(float fps);
  
  /**
   * Get current target FPS
   * @return Target frames per second
   */
  float getTargetFPS() const;
  
  /**
   * Get total number of clients served since startup
   * @return Client count
   */
  uint32_t getClientCount() const;
  
  /**
   * Get total frames sent to all clients
   * @return Frame count
   */
  uint32_t getFrameCount() const;
  
  /**
   * Get total bytes sent to all clients
   * @return Byte count
   */
  uint32_t getBytesSent() const;
  
  /**
   * Get actual FPS (calculated over last 10 frames)
   * @return Actual frames per second
   */
  float getActualFPS();
  
  /**
   * Disconnect current client
   */
  void disconnectClient();

private:
  WiFiServer _server;
  WiFiClient _client;
  
  uint16_t _port;
  float _targetFPS;
  unsigned long _frameInterval;
  unsigned long _lastFrameTime;
  
  // Statistics
  uint32_t _clientCount;
  uint32_t _frameCount;
  uint32_t _bytesSent;
  
  // FPS calculation
  unsigned long _fpsTimestamps[10];
  uint8_t _fpsIndex;
  
  void updateFPS();
};

#endif // TCP_SERVER_H
