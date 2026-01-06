/**
 * UDPNetworkClient - UDP implementation of NetworkClient with fragmentation
 *
 * Splits large frames into fragments to avoid IP fragmentation.
 * Fragment header format (12 bytes):
 *   [4 bytes: frame_id]
 *   [2 bytes: fragment_index]
 *   [2 bytes: total_fragments]
 *   [4 bytes: total_frame_size]
 *   [payload: up to MAX_PAYLOAD_SIZE bytes]
 */

#ifndef UDP_NETWORK_CLIENT_H
#define UDP_NETWORK_CLIENT_H

#include "NetworkClient.h"
#include <WiFi.h>
#include <WiFiUdp.h>

class UDPNetworkClient : public NetworkClient {
public:
    static const size_t HEADER_SIZE = 12;
    static const size_t MAX_PAYLOAD_SIZE = 1400;  // Stay under MTU
    static const size_t HANDSHAKE_SIZE = 10;      // 2 magic + 8 password

    // Default password (can be changed via setPassword)
    UDPNetworkClient() : _connected(false), _frameId(0) {
        // Default 8-byte password
        memcpy(_password, "ESP32CAM", 8);
    }

    /**
     * Set the 8-byte handshake password
     */
    void setPassword(const char* password) {
        memset(_password, 0, 8);
        strncpy((char*)_password, password, 8);
    }

    bool connect(const char* host, uint16_t port) override {
        _host = host;
        _port = port;

        // Resolve hostname to IP
        IPAddress ip;
        if (WiFi.hostByName(host, ip)) {
            _remoteIP = ip;
            _connected = true;
            _frameId = 0;  // Reset frame counter on connect

            // Send handshake packet
            sendHandshake();

            return true;
        }
        return false;
    }

    /**
     * Send handshake to reset server state
     */
    void sendHandshake() {
        if (!_connected) return;

        _udp.beginPacket(_remoteIP, _port);

        // Magic bytes "HS"
        _udp.write((uint8_t)'H');
        _udp.write((uint8_t)'S');

        // 8-byte password
        _udp.write(_password, 8);

        _udp.endPacket();

        // Reset local frame counter
        _frameId = 0;
    }

    bool connected() override {
        return _connected;
    }

    size_t send(const uint8_t* data, size_t len) override {
        if (!_connected) return 0;

        // Calculate number of fragments needed
        uint16_t totalFragments = (len + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
        if (totalFragments == 0) totalFragments = 1;

        size_t offset = 0;
        uint16_t fragmentIndex = 0;

        while (offset < len) {
            size_t payloadSize = min(MAX_PAYLOAD_SIZE, len - offset);

            _udp.beginPacket(_remoteIP, _port);

            // Write header (all big-endian)
            // Frame ID (4 bytes)
            uint32_t frameIdBE = htonl(_frameId);
            _udp.write((uint8_t*)&frameIdBE, 4);

            // Fragment index (2 bytes)
            uint16_t fragIdxBE = htons(fragmentIndex);
            _udp.write((uint8_t*)&fragIdxBE, 2);

            // Total fragments (2 bytes)
            uint16_t totalFragBE = htons(totalFragments);
            _udp.write((uint8_t*)&totalFragBE, 2);

            // Total frame size (4 bytes)
            uint32_t totalSizeBE = htonl((uint32_t)len);
            _udp.write((uint8_t*)&totalSizeBE, 4);

            // Write payload
            _udp.write(data + offset, payloadSize);

            if (!_udp.endPacket()) {
                return 0;  // Send failed
            }

            offset += payloadSize;
            fragmentIndex++;
        }

        _frameId++;
        return len;
    }

    void stop() override {
        _udp.stop();
        _connected = false;
    }

    // No-ops for UDP
    void setNoDelay(bool nodelay) override {}
    void setTimeout(uint32_t timeout) override {}

private:
    WiFiUDP _udp;
    const char* _host;
    uint16_t _port;
    IPAddress _remoteIP;
    bool _connected;
    uint32_t _frameId;
    uint8_t _password[8];

    uint32_t htonl(uint32_t hostlong) {
        return ((hostlong & 0xFF000000) >> 24) |
               ((hostlong & 0x00FF0000) >> 8) |
               ((hostlong & 0x0000FF00) << 8) |
               ((hostlong & 0x000000FF) << 24);
    }

    uint16_t htons(uint16_t hostshort) {
        return ((hostshort & 0xFF00) >> 8) |
               ((hostshort & 0x00FF) << 8);
    }
};

#endif // UDP_NETWORK_CLIENT_H
