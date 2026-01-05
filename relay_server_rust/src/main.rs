//! ESP32-CAM Relay Server (Receiver Mode)
//!
//! Receives JPEG frames pushed by ESP32-CAM and broadcasts to multiple clients.
//!
//! Architecture:
//!     ESP32-CAM (Client) -> [Port 4444] -> Relay Server -> [Port 8080] -> Internet Clients
//!          Pushes frames                   Receives &                     Multiple viewers
//!                                          Broadcasts

use std::net::SocketAddr;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::Arc;
use std::time::Instant;

use chrono::Local;
use clap::Parser;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{TcpListener, TcpStream};
use tokio::sync::{broadcast, watch, RwLock};

const JPEG_START: [u8; 2] = [0xFF, 0xD8];
const JPEG_END: [u8; 2] = [0xFF, 0xD9];
const BUFFER_SIZE: usize = 8192;
const MAX_BUFFER_SIZE: usize = 500_000;
const BROADCAST_CHANNEL_SIZE: usize = 16;

#[derive(Parser, Debug)]
#[command(name = "relay_server_receiver")]
#[command(about = "ESP32-CAM Receiver Relay Server (Push Mode)")]
#[command(after_help = r#"
Examples:
  # Basic usage
  relay_server_receiver --sender-port 4444 --client-port 8080

  # Listen on specific interface
  relay_server_receiver --sender-host 0.0.0.0 --sender-port 4444 --client-port 8080

Setup:
  1. Run this relay server on your VPS
  2. Configure ESP32-CAM with main_client.cpp
  3. Set RELAY_HOST to your VPS IP in ESP32-CAM code
  4. Upload to ESP32-CAM
  5. Clients connect to VPS:8080
"#)]
struct Args {
    /// Interface to listen for ESP32-CAM
    #[arg(long, default_value = "0.0.0.0")]
    sender_host: String,

    /// Port for ESP32-CAM to connect to
    #[arg(long, default_value_t = 4444)]
    sender_port: u16,

    /// Interface to listen for clients
    #[arg(long, default_value = "0.0.0.0")]
    client_host: String,

    /// Port for clients to connect to
    #[arg(long, default_value_t = 8080)]
    client_port: u16,

    /// Enable debug logging
    #[arg(long)]
    debug: bool,
}

struct Stats {
    total_frames: AtomicU64,
    total_bytes: AtomicU64,
    active_clients: AtomicU64,
    start_time: Instant,
}

impl Stats {
    fn new() -> Self {
        Self {
            total_frames: AtomicU64::new(0),
            total_bytes: AtomicU64::new(0),
            active_clients: AtomicU64::new(0),
            start_time: Instant::now(),
        }
    }

    fn add_frame(&self, bytes: u64) {
        self.total_frames.fetch_add(1, Ordering::Relaxed);
        self.total_bytes.fetch_add(bytes, Ordering::Relaxed);
    }

    fn fps(&self) -> f64 {
        let elapsed = self.start_time.elapsed().as_secs_f64();
        if elapsed > 0.0 {
            self.total_frames.load(Ordering::Relaxed) as f64 / elapsed
        } else {
            0.0
        }
    }
}

fn log_info(msg: &str) {
    println!("{} - INFO - {}", Local::now().format("%Y-%m-%d %H:%M:%S"), msg);
}

fn log_error(msg: &str) {
    eprintln!("{} - ERROR - {}", Local::now().format("%Y-%m-%d %H:%M:%S"), msg);
}

/// Find JPEG markers in buffer
fn find_jpeg_frame(buffer: &[u8]) -> Option<(usize, usize)> {
    // Find start marker
    let start = buffer.windows(2).position(|w| w == JPEG_START)?;

    // Find end marker after start
    let end_search_start = start + 2;
    if end_search_start >= buffer.len() {
        return None;
    }

    let end = buffer[end_search_start..]
        .windows(2)
        .position(|w| w == JPEG_END)
        .map(|pos| end_search_start + pos + 2)?;

    Some((start, end))
}

async fn handle_esp32_connection(
    mut socket: TcpStream,
    addr: SocketAddr,
    frame_tx: broadcast::Sender<Arc<Vec<u8>>>,
    latest_frame: Arc<RwLock<Option<Arc<Vec<u8>>>>>,
    stats: Arc<Stats>,
    running: Arc<AtomicBool>,
) {
    log_info(&format!("ESP32-CAM connected from {}", addr));

    // Set TCP_NODELAY for lower latency
    let _ = socket.set_nodelay(true);

    let mut buffer = Vec::with_capacity(MAX_BUFFER_SIZE);
    let mut read_buf = [0u8; BUFFER_SIZE];
    let mut local_frame_count = 0u64;

    while running.load(Ordering::Relaxed) {
        match socket.read(&mut read_buf).await {
            Ok(0) => {
                log_info(&format!("ESP32-CAM {} disconnected", addr));
                break;
            }
            Ok(n) => {
                buffer.extend_from_slice(&read_buf[..n]);

                // Extract complete JPEG frames
                while let Some((start, end)) = find_jpeg_frame(&buffer) {
                    let frame_data: Vec<u8> = buffer[start..end].to_vec();
                    let frame_len = frame_data.len();

                    // Remove processed data from buffer
                    buffer.drain(..end);

                    // Update stats
                    local_frame_count += 1;
                    stats.add_frame(frame_len as u64);
                    let fps = stats.fps();
                    let total = stats.total_frames.load(Ordering::Relaxed);

                    log_info(&format!(
                        "Frame #{}: {} bytes ({:.1} KB, {:.2} fps avg)",
                        total,
                        frame_len,
                        frame_len as f64 / 1024.0,
                        fps
                    ));

                    // Wrap in Arc for efficient sharing
                    let frame_arc = Arc::new(frame_data);

                    // Update latest frame
                    {
                        let mut latest = latest_frame.write().await;
                        *latest = Some(Arc::clone(&frame_arc));
                    }

                    // Broadcast to clients (ignore errors - no receivers is ok)
                    let _ = frame_tx.send(frame_arc);
                }

                // Safety: prevent buffer from growing too large
                if buffer.len() > MAX_BUFFER_SIZE {
                    log_info(&format!("Buffer too large ({} bytes), resetting", buffer.len()));
                    // Keep data from last potential JPEG start
                    if let Some(pos) = buffer.windows(2).rposition(|w| w == JPEG_START) {
                        buffer.drain(..pos);
                    } else {
                        buffer.clear();
                    }
                }
            }
            Err(e) => {
                log_error(&format!("Error receiving from ESP32-CAM {}: {}", addr, e));
                break;
            }
        }
    }

    log_info(&format!(
        "ESP32-CAM {} connection closed. Frames received: {}",
        addr, local_frame_count
    ));
}

async fn handle_client_connection(
    mut socket: TcpStream,
    addr: SocketAddr,
    mut frame_rx: broadcast::Receiver<Arc<Vec<u8>>>,
    latest_frame: Arc<RwLock<Option<Arc<Vec<u8>>>>>,
    stats: Arc<Stats>,
    running: Arc<AtomicBool>,
) {
    log_info(&format!("Client mamma connected from {}", addr));
    stats.active_clients.fetch_add(1, Ordering::Relaxed);
    log_info(&format!("Active clients: {}", stats.active_clients.load(Ordering::Relaxed)));

    // Set TCP_NODELAY for lower latency
    let _ = socket.set_nodelay(true);

    // Send latest frame immediately if available
    {
        let latest = latest_frame.read().await;
        if let Some(ref frame) = *latest {
            if let Err(e) = socket.write_all(frame).await {
                log_error(&format!("Failed to send cached frame to {}: {}", addr, e));
                stats.active_clients.fetch_sub(1, Ordering::Relaxed);
                return;
            }
            log_info(&format!("Sent cached frame ({} bytes) to {}", frame.len(), addr));
        }
    }

    // Receive and forward frames
    while running.load(Ordering::Relaxed) {
        match frame_rx.recv().await {
            Ok(frame) => {
                if let Err(e) = socket.write_all(&frame).await {
                    log_info(&format!("Client {} write error: {}", addr, e));
                    break;
                }
            }
            Err(broadcast::error::RecvError::Lagged(n)) => {
                // Client is too slow, skip frames
                log_info(&format!("Client {} lagged {} frames", addr, n));
            }
            Err(broadcast::error::RecvError::Closed) => {
                break;
            }
        }
    }

    stats.active_clients.fetch_sub(1, Ordering::Relaxed);
    log_info(&format!(
        "Client {} disconnected. Active clients: {}",
        addr,
        stats.active_clients.load(Ordering::Relaxed)
    ));
}

async fn run_sender_server(
    host: String,
    port: u16,
    frame_tx: broadcast::Sender<Arc<Vec<u8>>>,
    latest_frame: Arc<RwLock<Option<Arc<Vec<u8>>>>>,
    stats: Arc<Stats>,
    running: Arc<AtomicBool>,
    mut shutdown_rx: watch::Receiver<bool>,
) -> std::io::Result<()> {
    let addr = format!("{}:{}", host, port);
    let listener = TcpListener::bind(&addr).await?;

    log_info(&format!("ESP32-CAM server listening on {}", addr));

    loop {
        tokio::select! {
            result = listener.accept() => {
                match result {
                    Ok((socket, addr)) => {
                        let frame_tx = frame_tx.clone();
                        let latest_frame = Arc::clone(&latest_frame);
                        let stats = Arc::clone(&stats);
                        let running = Arc::clone(&running);

                        tokio::spawn(async move {
                            handle_esp32_connection(socket, addr, frame_tx, latest_frame, stats, running).await;
                        });
                    }
                    Err(e) => {
                        if running.load(Ordering::Relaxed) {
                            log_error(&format!("Error accepting ESP32-CAM: {}", e));
                        }
                    }
                }
            }
            _ = shutdown_rx.changed() => {
                break;
            }
        }
    }

    Ok(())
}

async fn run_client_server(
    host: String,
    port: u16,
    frame_tx: broadcast::Sender<Arc<Vec<u8>>>,
    latest_frame: Arc<RwLock<Option<Arc<Vec<u8>>>>>,
    stats: Arc<Stats>,
    running: Arc<AtomicBool>,
    mut shutdown_rx: watch::Receiver<bool>,
) -> std::io::Result<()> {
    let addr = format!("{}:{}", host, port);
    let listener = TcpListener::bind(&addr).await?;

    log_info(&format!("Client server listening on {}", addr));

    loop {
        tokio::select! {
            result = listener.accept() => {
                match result {
                    Ok((socket, addr)) => {
                        let frame_rx = frame_tx.subscribe();
                        let latest_frame = Arc::clone(&latest_frame);
                        let stats = Arc::clone(&stats);
                        let running = Arc::clone(&running);

                        tokio::spawn(async move {
                            handle_client_connection(socket, addr, frame_rx, latest_frame, stats, running).await;
                        });
                    }
                    Err(e) => {
                        if running.load(Ordering::Relaxed) {
                            log_error(&format!("Error accepting client: {}", e));
                        }
                    }
                }
            }
            _ = shutdown_rx.changed() => {
                break;
            }
        }
    }

    Ok(())
}

async fn print_stats(stats: Arc<Stats>, running: Arc<AtomicBool>) {
    let mut interval = tokio::time::interval(std::time::Duration::from_secs(30));

    while running.load(Ordering::Relaxed) {
        interval.tick().await;

        let total_frames = stats.total_frames.load(Ordering::Relaxed);
        if total_frames > 0 {
            let elapsed = stats.start_time.elapsed().as_secs_f64();
            let total_bytes = stats.total_bytes.load(Ordering::Relaxed);
            let avg_fps = if elapsed > 0.0 { total_frames as f64 / elapsed } else { 0.0 };
            let avg_size = total_bytes as f64 / total_frames as f64;
            let active = stats.active_clients.load(Ordering::Relaxed);

            log_info(&"=".repeat(60));
            log_info("Statistics:");
            log_info(&format!("  Uptime: {:.1} minutes", elapsed / 60.0));
            log_info(&format!("  Total frames received: {}", total_frames));
            log_info(&format!("  Total data: {:.2} MB", total_bytes as f64 / 1024.0 / 1024.0));
            log_info(&format!("  Average FPS: {:.2}", avg_fps));
            log_info(&format!("  Average frame size: {:.1} KB", avg_size / 1024.0));
            log_info(&format!("  Active clients: {}", active));
            log_info(&"=".repeat(60));
        }
    }
}

#[tokio::main]
async fn main() -> std::io::Result<()> {
    let args = Args::parse();

    // Print banner
    log_info(&"=".repeat(70));
    log_info("ESP32-CAM Receiver Relay Server (Push Mode) - Rust Edition");
    log_info(&"=".repeat(70));
    log_info("");
    log_info("Configuration:");
    log_info(&format!("  ESP32-CAM port (receives): {}:{}", args.sender_host, args.sender_port));
    log_info(&format!("  Client port (serves): {}:{}", args.client_host, args.client_port));
    log_info("");
    log_info("How it works:");
    log_info(&format!("  1. ESP32-CAM connects and pushes frames to port {}", args.sender_port));
    log_info("  2. Relay receives and stores latest frame");
    log_info(&format!("  3. Internet clients connect to port {}", args.client_port));
    log_info("  4. Relay broadcasts frames to all connected clients");
    log_info("");
    log_info("ESP32-CAM Setup:");
    log_info("  - Use main_client.cpp instead of main.cpp");
    log_info("  - Set RELAY_HOST to this server's IP");
    log_info(&format!("  - Set RELAY_PORT to {}", args.sender_port));
    log_info("");
    log_info(&format!("Client Usage: nc SERVER_IP {} > frame.jpg", args.client_port));
    log_info("");
    log_info("Press Ctrl+C to stop");
    log_info(&"=".repeat(70));
    log_info("");

    // Shared state
    let (frame_tx, _) = broadcast::channel::<Arc<Vec<u8>>>(BROADCAST_CHANNEL_SIZE);
    let latest_frame: Arc<RwLock<Option<Arc<Vec<u8>>>>> = Arc::new(RwLock::new(None));
    let stats = Arc::new(Stats::new());
    let running = Arc::new(AtomicBool::new(true));
    let (shutdown_tx, shutdown_rx) = watch::channel(false);

    // Handle Ctrl+C
    let running_ctrlc = Arc::clone(&running);
    tokio::spawn(async move {
        tokio::signal::ctrl_c().await.ok();
        log_info("\nShutting down...");
        running_ctrlc.store(false, Ordering::Relaxed);
        let _ = shutdown_tx.send(true);
    });

    // Start stats printer
    let stats_clone = Arc::clone(&stats);
    let running_clone = Arc::clone(&running);
    tokio::spawn(async move {
        print_stats(stats_clone, running_clone).await;
    });

    // Start sender server (ESP32-CAM connections)
    let frame_tx_sender = frame_tx.clone();
    let latest_frame_sender = Arc::clone(&latest_frame);
    let stats_sender = Arc::clone(&stats);
    let running_sender = Arc::clone(&running);
    let shutdown_rx_sender = shutdown_rx.clone();
    let sender_handle = tokio::spawn(async move {
        if let Err(e) = run_sender_server(
            args.sender_host,
            args.sender_port,
            frame_tx_sender,
            latest_frame_sender,
            stats_sender,
            running_sender,
            shutdown_rx_sender,
        ).await {
            log_error(&format!("Sender server error: {}", e));
        }
    });

    // Start client server (viewer connections)
    let stats_client = Arc::clone(&stats);
    let running_client = Arc::clone(&running);
    let client_handle = tokio::spawn(async move {
        if let Err(e) = run_client_server(
            args.client_host,
            args.client_port,
            frame_tx,
            latest_frame,
            stats_client,
            running_client,
            shutdown_rx,
        ).await {
            log_error(&format!("Client server error: {}", e));
        }
    });

    // Wait for tasks
    let _ = tokio::join!(sender_handle, client_handle);

    log_info("Server stopped");
    Ok(())
}
