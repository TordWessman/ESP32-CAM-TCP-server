#!/usr/bin/env python3
"""
Simple client to test ESP32-CAM connection and save frames
Works with both local and remote (relayed) connections

Usage:
    # Local connection
    python3 test_client.py --host 192.168.1.100 --port 1234
    
    # Remote connection (via relay)
    python3 test_client.py --host your-server.com --port 8080
    
    # Continuous capture
    python3 test_client.py --host 192.168.1.100 --port 1234 --continuous --fps 5
"""

import socket
import argparse
import time
import sys
from datetime import datetime
import os

def capture_frame(host, port, output_file, timeout=10):
    """Capture a single frame from ESP32-CAM"""
    try:
        # Connect to ESP32-CAM
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        
        print(f"Connecting to {host}:{port}...", end=' ')
        sock.connect((host, port))
        print("✓ Connected")
        
        # Receive data
        print("Receiving image data...", end=' ')
        data = b''
        start_time = time.time()
        
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
            
            # Safety: stop if we've received > 1MB (something's wrong)
            if len(data) > 1024 * 1024:
                print("⚠ Data too large, stopping")
                break
        
        elapsed = time.time() - start_time
        
        if data:
            # Save to file
            with open(output_file, 'wb') as f:
                f.write(data)
            
            print(f"✓ Saved {len(data)} bytes to {output_file}")
            print(f"  Transfer time: {elapsed:.2f}s ({len(data)/elapsed/1024:.1f} KB/s)")
            return True
        else:
            print("✗ No data received")
            return False
            
    except socket.timeout:
        print(f"✗ Connection timeout after {timeout}s")
        return False
    except ConnectionRefusedError:
        print(f"✗ Connection refused - is ESP32-CAM running?")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False
    finally:
        try:
            sock.close()
        except:
            pass

def continuous_capture(host, port, output_dir, fps, duration=None):
    """Continuously capture frames"""
    os.makedirs(output_dir, exist_ok=True)
    
    interval = 1.0 / fps
    frame_count = 0
    start_time = time.time()
    
    print("="*50)
    print(f"Continuous Capture Mode")
    print(f"Target FPS: {fps}")
    print(f"Output directory: {output_dir}")
    if duration:
        print(f"Duration: {duration} seconds")
    print("Press Ctrl+C to stop")
    print("="*50)
    print()
    
    try:
        while True:
            if duration and (time.time() - start_time) > duration:
                break
            
            frame_start = time.time()
            
            # Capture frame
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
            output_file = os.path.join(output_dir, f"frame_{timestamp}.jpg")
            
            if capture_frame(host, port, output_file):
                frame_count += 1
                
                # Calculate actual FPS
                elapsed = time.time() - start_time
                actual_fps = frame_count / elapsed if elapsed > 0 else 0
                
                print(f"Frame {frame_count} | Actual FPS: {actual_fps:.2f}")
            else:
                print("Failed to capture frame, retrying...")
            
            # Wait for next frame
            frame_time = time.time() - frame_start
            sleep_time = max(0, interval - frame_time)
            if sleep_time > 0:
                time.sleep(sleep_time)
            
    except KeyboardInterrupt:
        print("\n\nStopping...")
    
    # Summary
    total_time = time.time() - start_time
    avg_fps = frame_count / total_time if total_time > 0 else 0
    
    print()
    print("="*50)
    print("Capture Summary")
    print("="*50)
    print(f"Total frames: {frame_count}")
    print(f"Total time: {total_time:.2f}s")
    print(f"Average FPS: {avg_fps:.2f}")
    print(f"Output directory: {output_dir}")
    print("="*50)

def main():
    parser = argparse.ArgumentParser(description='ESP32-CAM Test Client')
    parser.add_argument('--host', required=True,
                      help='ESP32-CAM hostname or IP')
    parser.add_argument('--port', type=int, default=1234,
                      help='ESP32-CAM port (default: 1234)')
    parser.add_argument('--output', default='frame.jpg',
                      help='Output filename (default: frame.jpg)')
    parser.add_argument('--continuous', action='store_true',
                      help='Continuous capture mode')
    parser.add_argument('--fps', type=float, default=5,
                      help='Target FPS for continuous mode (default: 5)')
    parser.add_argument('--duration', type=float,
                      help='Duration in seconds for continuous mode')
    parser.add_argument('--output-dir', default='frames',
                      help='Output directory for continuous mode (default: frames)')
    parser.add_argument('--timeout', type=float, default=10,
                      help='Connection timeout in seconds (default: 10)')
    
    args = parser.parse_args()
    
    if args.continuous:
        continuous_capture(args.host, args.port, args.output_dir, args.fps, args.duration)
    else:
        success = capture_frame(args.host, args.port, args.output, args.timeout)
        sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
