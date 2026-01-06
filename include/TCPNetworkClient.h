/**
 * TCPNetworkClient - TCP implementation of NetworkClient
 */

#ifndef TCP_NETWORK_CLIENT_H
#define TCP_NETWORK_CLIENT_H

#include "NetworkClient.h"
#include <WiFiClient.h>

class TCPNetworkClient : public NetworkClient {
public:
    TCPNetworkClient() : _connected(false) {}

    bool connect(const char* host, uint16_t port) override {
        _connected = _client.connect(host, port);
        return _connected;
    }

    bool connected() override {
        _connected = _client.connected();
        return _connected;
    }

    size_t send(const uint8_t* data, size_t len) override {
        return _client.write(data, len);
    }

    void stop() override {
        _client.stop();
        _connected = false;
    }

    void setNoDelay(bool nodelay) override {
        _client.setNoDelay(nodelay);
    }

    void setTimeout(uint32_t timeout) override {
        _client.setTimeout(timeout);
    }

private:
    WiFiClient _client;
    bool _connected;
};

#endif // TCP_NETWORK_CLIENT_H
