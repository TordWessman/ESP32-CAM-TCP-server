#!/usr/bin/env python3
"""ESP32-CAM Relay Server - Broadcasts frames to multiple clients"""

import socket
import threading
import argparse
import logging
import time

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class ESP32CamBroadcastRelay:
    def __init__(self, esp32_host, esp32_port, listen_host='0.0.0.0', listen_port=8080):
        self.esp32_host = esp32_host
        self.esp32_port = esp32_port
        self.listen_host = listen_host
        self.listen_port = listen_port
        self.clients = []
        self.clients_lock = threading.Lock()
        self.latest_frame = None
        self.latest_frame_lock = threading.Lock()
        self.running = True
        self.total_frames = 0
        
    def connect_to_esp32(self):
        while self.running:
            try:
                logger.info(f"Connecting to ESP32-CAM at {self.esp32_host}:{self.esp32_port}")
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(10.0)
                sock.connect((self.esp32_host, self.esp32_port))
                sock.settimeout(None)
                logger.info("Connected to ESP32-CAM")
                
                while self.running:
                    frame_data = b''
                    
                    # Read data until connection closes or we get a complete JPEG
                    while True:
                        chunk = sock.recv(4096)
                        if not chunk:
                            break
                        frame_data += chunk
                        
                        # Check if we have a complete JPEG (ends with 0xFF 0xD9)
                        if len(frame_data) > 2 and frame_data[-2:] == b'\xff\xd9':
                            break
                        
                        # Safety check
                        if len(frame_data) > 1024 * 1024:
                            raise Exception("Frame too large")
                    
                    if frame_data:
                        # Verify it's a valid JPEG (starts with 0xFF 0xD8)
                        if frame_data[:2] == b'\xff\xd8':
                            with self.latest_frame_lock:
                                self.latest_frame = frame_data
                            self.total_frames += 1
                            logger.info(f"Frame #{self.total_frames}: {len(frame_data)} bytes ({len(frame_data)/1024:.1f} KB)")
                            self.broadcast_frame(frame_data)
                        else:
                            logger.warning(f"Invalid JPEG data received (first bytes: {frame_data[:4].hex()})")
                    
                    sock.close()
                    time.sleep(0.05)
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(10.0)
                    sock.connect((self.esp32_host, self.esp32_port))
                    sock.settimeout(None)
                    
            except Exception as e:
                logger.error(f"Error: {e}")
                try:
                    sock.close()
                except:
                    pass
            if self.running:
                time.sleep(5)
    
    def broadcast_frame(self, frame_data):
        with self.clients_lock:
            dead_clients = []
            for client_socket, client_address in self.clients:
                try:
                    client_socket.sendall(frame_data)
                except:
                    dead_clients.append((client_socket, client_address))
            for client in dead_clients:
                try:
                    client[0].close()
                except:
                    pass
                self.clients.remove(client)
    
    def handle_client(self, client_socket, client_address):
        logger.info(f"Client connected: {client_address}")
        with self.clients_lock:
            self.clients.append((client_socket, client_address))
        with self.latest_frame_lock:
            if self.latest_frame:
                try:
                    client_socket.sendall(self.latest_frame)
                except:
                    pass
        while self.running:
            time.sleep(1)
    
    def start_client_server(self):
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((self.listen_host, self.listen_port))
        server_socket.listen(10)
        logger.info(f"Listening on {self.listen_host}:{self.listen_port}")
        while self.running:
            try:
                server_socket.settimeout(1.0)
                client_socket, client_address = server_socket.accept()
                threading.Thread(target=self.handle_client, args=(client_socket, client_address), daemon=True).start()
            except socket.timeout:
                continue
    
    def start(self):
        logger.info("Starting ESP32-CAM Relay Server")
        logger.info(f"ESP32-CAM: {self.esp32_host}:{self.esp32_port}")
        logger.info(f"Client port: {self.listen_port}")
        threading.Thread(target=self.connect_to_esp32, daemon=True).start()
        try:
            self.start_client_server()
        except KeyboardInterrupt:
            logger.info("Shutting down")
            self.running = False

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--esp32-host', required=True)
    parser.add_argument('--esp32-port', type=int, default=1234)
    parser.add_argument('--client-port', type=int, default=8080)
    args = parser.parse_args()
    relay = ESP32CamBroadcastRelay(args.esp32_host, args.esp32_port, listen_port=args.client_port)
    relay.start()
