/**
 * NetworkClient - Abstract interface for network communication
 *
 * Allows dependency injection of TCP or UDP implementations.
 */

#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <Arduino.h>

class NetworkClient {
public:
    virtual ~NetworkClient() {}

    /**
     * Connect to the remote host
     * @param host Hostname or IP address
     * @param port Port number
     * @return true if connection successful
     */
    virtual bool connect(const char* host, uint16_t port) = 0;

    /**
     * Check if connected/ready to send
     */
    virtual bool connected() = 0;

    /**
     * Send data
     * @param data Pointer to data buffer
     * @param len Length of data
     * @return Number of bytes sent
     */
    virtual size_t send(const uint8_t* data, size_t len) = 0;

    /**
     * Stop/close the connection
     */
    virtual void stop() = 0;

    /**
     * Set socket options (implementation-specific)
     */
    virtual void setNoDelay(bool nodelay) {}
    virtual void setTimeout(uint32_t timeout) {}
};

#endif // NETWORK_CLIENT_H
