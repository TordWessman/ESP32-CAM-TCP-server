/**
 * TcpServer - Implementation
 * 
 * Non-blocking TCP server with FPS control and client management.
 * Completely decoupled from camera or sensor code.
 */

#include "TcpServer.h"
#include <Arduino.h>

TcpServer::TcpServer(uint16_t port, float targetFPS)
  : _server(port),
    _port(port),
    _targetFPS(targetFPS),
    _frameInterval((unsigned long)(1000.0 / targetFPS)),
    _lastFrameTime(0),
    _clientCount(0),
    _frameCount(0),
    _bytesSent(0),
    _fpsIndex(0)
{
  // Initialize FPS timestamp array
  for (int i = 0; i < 10; i++) {
    _fpsTimestamps[i] = 0;
  }
}

void TcpServer::begin() {
  _server.begin();
  Serial.printf("[TcpServer] Started on port %d (target %.1f FPS)\n", _port, _targetFPS);
  Serial.printf("[TcpServer] Frame interval: %lu ms\n", _frameInterval);
}

void TcpServer::run() {
  // Check for new client if we don't have one
  if (!_client || !_client.connected()) {
    if (_client) {
      // Previous client disconnected
      Serial.println("[TcpServer] Client disconnected");
      _client.stop();
    }
    
    // Accept new client
    _client = _server.available();
    if (_client) {
      _clientCount++;
      Serial.printf("[TcpServer] New client connected: %s (Total clients: %u)\n", 
                    _client.remoteIP().toString().c_str(), _clientCount);
    }
  }
}

bool TcpServer::sendData(const uint8_t* data, size_t length) {
  if (!hasClient()) {
    return false;
  }
  
  size_t written = _client.write(data, length);
  
  if (written != length) {
    Serial.printf("[TcpServer] ERROR: Sent %u/%u bytes\n", written, length);
    disconnectClient();
    return false;
  }
  
  // Update statistics
  _bytesSent += written;
  _frameCount++;
  _lastFrameTime = millis();
  updateFPS();
  
  // Debug output every 100 frames
  if (_frameCount % 100 == 0) {
    Serial.printf("[TcpServer] Stats - Frames: %u, Bytes: %u, FPS: %.1f\n", 
                  _frameCount, _bytesSent, getActualFPS());
  }
  
  return true;
}

bool TcpServer::canSend() {
  unsigned long now = millis();
  
  // Handle overflow
  if (now < _lastFrameTime) {
    _lastFrameTime = now;
    return true;
  }
  
  return (now - _lastFrameTime) >= _frameInterval;
}

bool TcpServer::hasClient() {
  return _client && _client.connected();
}

void TcpServer::setTargetFPS(float fps) {
  _targetFPS = fps;
  _frameInterval = (unsigned long)(1000.0 / fps);
  Serial.printf("[TcpServer] FPS changed to %.1f (interval: %lu ms)\n", fps, _frameInterval);
}

float TcpServer::getTargetFPS() const {
  return _targetFPS;
}

uint32_t TcpServer::getClientCount() const {
  return _clientCount;
}

uint32_t TcpServer::getFrameCount() const {
  return _frameCount;
}

uint32_t TcpServer::getBytesSent() const {
  return _bytesSent;
}

float TcpServer::getActualFPS() {
  if (_fpsTimestamps[9] == 0) {
    return 0.0;  // Not enough samples yet
  }
  
  // Calculate FPS over last 10 frames
  unsigned long elapsed = _fpsTimestamps[_fpsIndex] - _fpsTimestamps[(_fpsIndex + 1) % 10];
  if (elapsed == 0) {
    return 0.0;
  }
  
  return 9000.0 / elapsed;  // 9 intervals between 10 samples, in milliseconds
}

void TcpServer::disconnectClient() {
  if (_client) {
    Serial.println("[TcpServer] Disconnecting client");
    _client.stop();
  }
}

void TcpServer::updateFPS() {
  _fpsTimestamps[_fpsIndex] = millis();
  _fpsIndex = (_fpsIndex + 1) % 10;
}
