/**
 * RelayClient.cpp
 * 
 * Implementation of non-blocking TCP relay client
 */

#include "RelayClient.h"

RelayClient::RelayClient(const char* host, uint16_t port, float targetFPS)
    : _host(host)
    , _port(port)
    , _targetFPS(targetFPS)
    , _retryDelay(5000)
    , _debug(false)
    , _isConnected(false)
    , _lastConnectionAttempt(0)
    , _lastSendTime(0)
    , _minSendInterval(0)
    , _frameCount(0)
    , _totalBytesSent(0)
    , _startTime(0)
{
    updateFPSInterval();
}

void RelayClient::begin() {
    _startTime = millis();
    _lastConnectionAttempt = 0;
    _frameCount = 0;
    _totalBytesSent = 0;
    
    debugPrintf("RelayClient initialized: %s:%d", _host, _port);
    if (_targetFPS > 0) {
        debugPrintf("Target FPS: %.1f (interval: %lu ms)", _targetFPS, _minSendInterval);
    }
}

void RelayClient::run() {
    // Check connection state
    if (_client.connected()) {
        if (!_isConnected) {
            _isConnected = true;
            debugPrint("Connected to relay server");
        }
    } else {
        if (_isConnected) {
            _isConnected = false;
            debugPrint("Disconnected from relay server");
            _client.stop();
        }
        
        // Attempt reconnection with delay
        unsigned long now = millis();
        if (now - _lastConnectionAttempt >= _retryDelay) {
            attemptConnection();
        }
    }
}

bool RelayClient::sendData(const uint8_t* data, size_t length) {
    if (!_isConnected) {
        debugPrint("Cannot send: not connected");
        return false;
    }
    
    // Check FPS throttling
    if (!canSend()) {
        return false;
    }
    
    // Send data
    size_t totalSent = 0;
    size_t remaining = length;
    const uint8_t* ptr = data;
    
    while (remaining > 0 && _client.connected()) {
        size_t sent = _client.write(ptr, remaining);
        if (sent == 0) {
            debugPrint("Send failed: write returned 0");
            _client.stop();
            _isConnected = false;
            return false;
        }
        totalSent += sent;
        remaining -= sent;
        ptr += sent;
    }
    
    // Flush to ensure data is sent
    _client.flush();
    
    // Update statistics
    _frameCount++;
    _totalBytesSent += totalSent;
    _lastSendTime = millis();
    
    if (_debug) {
        debugPrintf("Sent frame #%lu: %d bytes (FPS: %.2f)", 
                   _frameCount, length, getActualFPS());
    }
    
    // Close connection after sending (relay server expects this)
    _client.stop();
    _isConnected = false;
    
    return true;
}

bool RelayClient::isConnected() const {
    return _isConnected;
}

bool RelayClient::canSend() const {
    if (_minSendInterval == 0) {
        return true;  // No FPS limit
    }
    
    unsigned long now = millis();
    return (now - _lastSendTime >= _minSendInterval);
}

void RelayClient::disconnect() {
    if (_client.connected()) {
        _client.stop();
    }
    _isConnected = false;
    debugPrint("Disconnected");
}

void RelayClient::setTargetFPS(float fps) {
    _targetFPS = fps;
    updateFPSInterval();
    
    if (_debug) {
        if (fps > 0) {
            debugPrintf("Target FPS set to %.1f (interval: %lu ms)", fps, _minSendInterval);
        } else {
            debugPrint("FPS limit disabled");
        }
    }
}

float RelayClient::getActualFPS() const {
    unsigned long elapsed = millis() - _startTime;
    if (elapsed == 0) {
        return 0.0;
    }
    return (_frameCount * 1000.0) / elapsed;
}

void RelayClient::attemptConnection() {
    _lastConnectionAttempt = millis();
    
    debugPrintf("Connecting to %s:%d...", _host, _port);
    
    if (_client.connect(_host, _port)) {
        _isConnected = true;
        debugPrint("Connected");
    } else {
        _isConnected = false;
        debugPrintf("Connection failed, retry in %lu ms", _retryDelay);
    }
}

void RelayClient::updateFPSInterval() {
    if (_targetFPS > 0) {
        _minSendInterval = (unsigned long)(1000.0 / _targetFPS);
    } else {
        _minSendInterval = 0;
    }
}

void RelayClient::debugPrint(const char* message) {
    if (_debug) {
        Serial.print("[RelayClient] ");
        Serial.println(message);
    }
}

void RelayClient::debugPrintf(const char* format, ...) {
    if (_debug) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.print("[RelayClient] ");
        Serial.println(buffer);
    }
}
