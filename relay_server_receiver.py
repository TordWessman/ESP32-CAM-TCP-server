#!/usr/bin/env python3
"""
Relay Server for ESP32-CAM Client Mode (Receiver Mode)

This server receives frames pushed by ESP32-CAM and broadcasts to multiple clients.

Architecture:
    ESP32-CAM (Client) → [Port 4444] → Relay Server → [Port 8080] → Internet Clients
         Pushes frames                  Receives &                    Multiple viewers
                                       Broadcasts

Usage:
    python3 relay_server_receiver.py --sender-port 4444 --client-port 8080

Difference from relay_server.py:
    - relay_server.py: Relay connects TO ESP32-CAM (pulls frames)
    - relay_server_receiver.py: ESP32-CAM connects TO relay (pushes frames)
"""

import socket
import threading
import argparse
import logging
import time

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class ESP32CamReceiverRelay:
    """
    Relay server that receives frames from ESP32-CAM and broadcasts to clients
    """
    
    def __init__(self, sender_host='0.0.0.0', sender_port=4444, client_host='0.0.0.0', client_port=8080):
        self.sender_host = sender_host
        self.sender_port = sender_port
        self.client_host = client_host
        self.client_port = client_port
        
        self.latest_frame = None
        self.latest_frame_lock = threading.Lock()
        self.clients = []
        self.clients_lock = threading.Lock()
        self.running = True
        
        # Statistics
        self.total_frames = 0
        self.total_bytes = 0
        self.start_time = time.time()
        
    def handle_esp32_connection(self, conn, addr):
        """Handle incoming connection from ESP32-CAM (continuous stream)"""
        logger.info(f"ESP32-CAM connected from {addr}")
        
        buffer = b''
        frame_count = 0
        
        try:
            while self.running:
                # Receive data in chunks
                chunk = conn.recv(4096)
                if not chunk:
                    logger.info(f"ESP32-CAM {addr} disconnected")
                    break
                
                buffer += chunk
                
                # Look for JPEG frames in the buffer
                # JPEG starts with 0xFF 0xD8 and ends with 0xFF 0xD9
                while True:
                    # Find start of JPEG
                    start_idx = buffer.find(b'\xff\xd8')
                    if start_idx == -1:
                        # No JPEG start found, keep first byte in case it's 0xFF
                        buffer = buffer[-1:] if buffer else b''
                        break
                    
                    # Find end of JPEG (after the start)
                    end_idx = buffer.find(b'\xff\xd9', start_idx + 2)
                    if end_idx == -1:
                        # Incomplete JPEG, wait for more data
                        # But discard any data before the JPEG start
                        buffer = buffer[start_idx:]
                        break
                    
                    # Extract complete JPEG frame (include the end marker)
                    frame_data = buffer[start_idx:end_idx + 2]
                    
                    # Remove processed frame from buffer
                    buffer = buffer[end_idx + 2:]
                    
                    # Process the frame
                    frame_count += 1
                    self.total_frames += 1
                    self.total_bytes += len(frame_data)
                    
                    fps = self.total_frames / (time.time() - self.start_time)
                    
                    logger.info(f"Frame #{self.total_frames}: {len(frame_data)} bytes "
                              f"({len(frame_data)/1024:.1f} KB, {fps:.2f} fps avg)")
                    
                    # Update latest frame
                    with self.latest_frame_lock:
                        self.latest_frame = frame_data
                    
                    # Broadcast to all clients
                    self.broadcast_frame(frame_data)
                    
                    # Safety check - prevent buffer from growing too large
                    if len(buffer) > 500000:  # 500KB
                        logger.warning(f"Buffer too large ({len(buffer)} bytes), resetting")
                        buffer = b''
                        break
            
        except Exception as e:
            logger.error(f"Error receiving from ESP32-CAM {addr}: {e}")
        finally:
            conn.close()
            logger.info(f"ESP32-CAM {addr} connection closed. Total frames received: {frame_count}")
    
    def broadcast_frame(self, frame_data):
        """Send frame to all connected clients"""
        with self.clients_lock:
            if not self.clients:
                return
            
            dead_clients = []
            
            for client_socket, client_address in self.clients:
                try:
                    client_socket.sendall(frame_data)
                except Exception as e:
                    logger.debug(f"Client {client_address} error: {e}")
                    dead_clients.append((client_socket, client_address))
            
            # Remove dead clients
            for client in dead_clients:
                try:
                    client[0].close()
                except:
                    pass
                self.clients.remove(client)
                logger.info(f"Client {client[1]} disconnected")
            
            if dead_clients:
                logger.info(f"Active clients: {len(self.clients)}")
    
    def handle_client_connection(self, conn, addr):
        """Handle incoming connection from viewer client"""
        logger.info(f"Client connected from {addr}")
        
        try:
            # Add to client list
            with self.clients_lock:
                self.clients.append((conn, addr))
                logger.info(f"Active clients: {len(self.clients)}")
            
            # Send latest frame immediately if available
            with self.latest_frame_lock:
                if self.latest_frame:
                    try:
                        conn.sendall(self.latest_frame)
                        logger.info(f"Sent cached frame ({len(self.latest_frame)} bytes) to {addr}")
                    except:
                        pass
            
            # Keep connection alive (frames sent by broadcast_frame)
            while self.running:
                try:
                    # Try to receive (will fail when client disconnects)
                    data = conn.recv(1)
                    if not data:
                        break
                except socket.timeout:
                    continue
                except:
                    break
                    
        except Exception as e:
            logger.debug(f"Client handler error for {addr}: {e}")
        finally:
            # Remove from client list
            with self.clients_lock:
                try:
                    self.clients.remove((conn, addr))
                    logger.info(f"Client {addr} removed. Active clients: {len(self.clients)}")
                except:
                    pass
            try:
                conn.close()
            except:
                pass
    
    def start_sender_server(self):
        """Accept connections from ESP32-CAM"""
        sender_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sender_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            sender_socket.bind((self.sender_host, self.sender_port))
            sender_socket.listen(5)
            
            logger.info(f"✓ ESP32-CAM server listening on {self.sender_host}:{self.sender_port}")
            
            while self.running:
                try:
                    sender_socket.settimeout(1.0)
                    conn, addr = sender_socket.accept()
                    
                    # Handle each ESP32-CAM connection in separate thread
                    threading.Thread(
                        target=self.handle_esp32_connection,
                        args=(conn, addr),
                        daemon=True
                    ).start()
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.running:
                        logger.error(f"Error accepting ESP32-CAM: {e}")
                        
        finally:
            sender_socket.close()
    
    def start_client_server(self):
        """Accept connections from viewer clients"""
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            client_socket.bind((self.client_host, self.client_port))
            client_socket.listen(10)
            
            logger.info(f"✓ Client server listening on {self.client_host}:{self.client_port}")
            
            while self.running:
                try:
                    client_socket.settimeout(1.0)
                    conn, addr = client_socket.accept()
                    
                    # Handle each client in separate thread
                    threading.Thread(
                        target=self.handle_client_connection,
                        args=(conn, addr),
                        daemon=True
                    ).start()
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.running:
                        logger.error(f"Error accepting client: {e}")
                        
        finally:
            client_socket.close()
    
    def print_stats(self):
        """Periodically print statistics"""
        while self.running:
            time.sleep(30)
            if self.total_frames > 0:
                elapsed = time.time() - self.start_time
                avg_fps = self.total_frames / elapsed if elapsed > 0 else 0
                avg_size = self.total_bytes / self.total_frames if self.total_frames > 0 else 0
                
                logger.info("="*60)
                logger.info("Statistics:")
                logger.info(f"  Uptime: {elapsed/60:.1f} minutes")
                logger.info(f"  Total frames received: {self.total_frames}")
                logger.info(f"  Total data: {self.total_bytes / 1024 / 1024:.2f} MB")
                logger.info(f"  Average FPS: {avg_fps:.2f}")
                logger.info(f"  Average frame size: {avg_size / 1024:.1f} KB")
                logger.info(f"  Active clients: {len(self.clients)}")
                logger.info("="*60)
    
    def start(self):
        """Start the relay server"""
        logger.info("="*70)
        logger.info("ESP32-CAM Receiver Relay Server (Push Mode)")
        logger.info("="*70)
        logger.info("")
        logger.info("Configuration:")
        logger.info(f"  ESP32-CAM port (receives): {self.sender_host}:{self.sender_port}")
        logger.info(f"  Client port (serves): {self.client_host}:{self.client_port}")
        logger.info("")
        logger.info("How it works:")
        logger.info("  1. ESP32-CAM connects and pushes frames to port " + str(self.sender_port))
        logger.info("  2. Relay receives and stores latest frame")
        logger.info("  3. Internet clients connect to port " + str(self.client_port))
        logger.info("  4. Relay broadcasts frames to all connected clients")
        logger.info("")
        logger.info("ESP32-CAM Setup:")
        logger.info("  - Use main_client.cpp instead of main.cpp")
        logger.info("  - Set RELAY_HOST to this server's IP")
        logger.info("  - Set RELAY_PORT to " + str(self.sender_port))
        logger.info("")
        logger.info("Client Usage:")
        logger.info(f"  nc SERVER_IP {self.client_port} > frame.jpg")
        logger.info("")
        logger.info("Press Ctrl+C to stop")
        logger.info("="*70)
        logger.info("")
        
        # Start statistics thread
        stats_thread = threading.Thread(target=self.print_stats, daemon=True)
        stats_thread.start()
        
        # Start ESP32-CAM server thread
        sender_thread = threading.Thread(target=self.start_sender_server, daemon=True)
        sender_thread.start()
        
        # Start client server (blocks in main thread)
        try:
            self.start_client_server()
        except KeyboardInterrupt:
            logger.info("\n\nShutting down...")
            self.running = False
        except Exception as e:
            logger.error(f"Server error: {e}")
            self.running = False
        finally:
            self.running = False
            time.sleep(1)
            logger.info("Server stopped")

def main():
    parser = argparse.ArgumentParser(
        description='ESP32-CAM Receiver Relay Server (for Client/Push Mode)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Basic usage
  python3 relay_server_receiver.py --sender-port 4444 --client-port 8080
  
  # Listen on specific interface
  python3 relay_server_receiver.py --sender-host 0.0.0.0 --sender-port 4444 --client-port 8080

Setup:
  1. Run this relay server on your VPS
  2. Configure ESP32-CAM with main_client.cpp
  3. Set RELAY_HOST to your VPS IP in ESP32-CAM code
  4. Upload to ESP32-CAM
  5. Clients connect to VPS:8080
        """
    )
    
    parser.add_argument('--sender-host', default='0.0.0.0',
                      help='Interface to listen for ESP32-CAM (default: 0.0.0.0)')
    parser.add_argument('--sender-port', type=int, default=4444,
                      help='Port for ESP32-CAM to connect to (default: 4444)')
    parser.add_argument('--client-host', default='0.0.0.0',
                      help='Interface to listen for clients (default: 0.0.0.0)')
    parser.add_argument('--client-port', type=int, default=8080,
                      help='Port for clients to connect to (default: 8080)')
    parser.add_argument('--debug', action='store_true',
                      help='Enable debug logging')
    
    args = parser.parse_args()
    
    if args.debug:
        logging.getLogger().setLevel(logging.DEBUG)
    
    relay = ESP32CamReceiverRelay(
        sender_host=args.sender_host,
        sender_port=args.sender_port,
        client_host=args.client_host,
        client_port=args.client_port
    )
    
    relay.start()

if __name__ == '__main__':
    main()
