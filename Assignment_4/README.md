# Video Streaming Simulation

This project implements a simulation of a video streaming service using both TCP and UDP protocols. The application consists of a Server and a Client that communicate in two distinct phases: Connection Phase and Video Streaming Phase.

## Overview

The application simulates video streaming over both TCP and UDP protocols, allowing for comparison of performance characteristics of both protocols. The server can handle multiple clients concurrently using threads.

## Features

- Server supports both TCP and UDP connections
- Multithreaded server to handle multiple clients simultaneously
- Two scheduling policies: FCFS (First-Come-First-Serve) and RR (Round-Robin)
- Different video resolutions with appropriate bandwidth requirements
- Dynamic TCP port allocation for safe concurrent TCP streaming
- Statistical tracking of connection performance (data rate, bytes sent, etc.)
- UDP packet loss detection and reporting
- Proper signal handling (including SIGPIPE) for robust operation
- Memory-safe implementation with proper resource management

## Requirements

- Unix/Linux/macOS operating system
- GCC compiler
- pthread library (usually included with the system)

## Compilation

To compile both the server and client programs, run:

```bash
make
```

This will create two executable files:
- `server` - The video streaming server
- `client` - The video streaming client

## Running the Application

### Server

Start the server first with the following command:

```bash
./server <Server Port> <Scheduling Policy>
```

- `<Server Port>`: The port number the server will listen on (e.g., 8080)
- `<Scheduling Policy>`: Either FCFS (First-Come-First-Serve) or RR (Round-Robin)

Example:
```bash
./server 8080 FCFS
```

### Client

Once the server is running, you can start one or more clients with the following command:

```bash
./client <Server IP> <Server Port> <Resolution> <Protocol>
```

- `<Server IP>`: The IP address of the server (e.g., 127.0.0.1 for localhost)
- `<Server Port>`: The port number the server is listening on
- `<Resolution>`: Video resolution, one of: 480p, 720p, or 1080p
- `<Protocol>`: Either TCP or UDP (case-insensitive)

Examples:
```bash
./client 127.0.0.1 8080 720p TCP
./client 127.0.0.1 8080 1080p UDP
./client 127.0.0.1 8080 480p TCP
```

## How It Works

### Connection Phase

1. The client initiates a TCP connection with the server
2. The client sends a Type 1 Request Message specifying the desired video resolution and protocol (TCP or UDP)
3. The server responds with a Type 2 Response Message including information about the video stream and bandwidth requirements
4. For TCP clients, the server dynamically assigns a streaming port

### Video Streaming Phase

1. After the connection phase, the client connects using the chosen protocol
2. For TCP Mode:
   - The client discovers the assigned streaming port
   - Video data is transmitted over a reliable TCP connection to avoid packet loss
   - The client confirms receipt of chunks and can detect if the stream ends
3. For UDP Mode:
   - Video data is sent using smaller datagrams (8KB) to avoid MTU issues
   - The client tracks packet loss and reports statistics
4. The server handles multiple clients concurrently based on the selected scheduling policy

## Viewing Statistics

### Server Statistics
To view streaming statistics from the server, press `Ctrl+C` in the server terminal. This sends a SIGINT signal which the server handles gracefully:
1. The server will display statistics for all connected clients
2. It will then terminate cleanly

Example output:
```
Server shutting down, collecting statistics...

----- Streaming Statistics -----
Total clients connected: 3

Client 0 (127.0.0.1): TCP - 720p
  Status: Finished
  Bytes sent: 13107200
  Chunks sent: 100
  Data rate: 224720.20 bytes/sec
  Elapsed time: 58.33 seconds

Client 1 (127.0.0.1): UDP - 1080p
  Status: Finished
  Bytes sent: 819200
  Chunks sent: 100
  Data rate: 16415.39 bytes/sec
  Elapsed time: 49.90 seconds

Client 2 (127.0.0.1): TCP - 480p
  Status: Streaming
  Bytes sent: 3145728
  Chunks sent: 24
  Data rate: 179328.51 bytes/sec
  Elapsed time: 17.54 seconds

------------------------------

Server terminated gracefully.
```

### Client Statistics

Each client automatically displays statistics every 10 chunks received:

- For TCP:
```
----- TCP Streaming Statistics -----
Resolution: 720p (Bandwidth: 3000 Kbps)
Chunks received: 10
Total data received: 1310720 bytes
Elapsed time: 5.84 seconds
Overall data rate: 224352.29 bytes/sec
Recent data rate: 224352.29 bytes/sec
------------------------------------
```

- For UDP (includes packet loss stats):
```
----- UDP Streaming Statistics -----
Resolution: 1080p (Bandwidth: 6000 Kbps)
Chunks received: 10
Total data received: 81920 bytes
Elapsed time: 5.05 seconds
Overall data rate: 16222.77 bytes/sec
Recent data rate: 16384 bytes/sec
Lost packets: 0
Packet loss rate: 0.00%
------------------------------------
```

## Scheduling Policies

### FCFS (First-Come-First-Serve)
- Clients are processed in the order they arrive
- Best for ensuring fair service based on arrival order

### RR (Round-Robin)
- Cycles through active clients to ensure all get service time
- Better for balancing resources among many clients

## Resolution and Bandwidth

The system supports three video resolutions with corresponding bandwidth requirements:
- 480p: 1.5 Mbps (1500 Kbps)
- 720p: 3 Mbps (3000 Kbps)
- 1080p: 6 Mbps (6000 Kbps)

## Protocol Comparison

### TCP
- Provides reliable, in-order delivery
- Uses larger chunk sizes (128KB)
- Dynamic port allocation for safe concurrent connections
- No packet loss, but potentially higher latency

### UDP
- Lower overhead, potentially faster delivery
- Uses smaller chunk sizes (8KB) to avoid datagram size limits
- May experience packet loss (tracked and reported in statistics)
- Better simulates real-time streaming behavior

## Troubleshooting

1. **Socket binding issues**: If you see "Address already in use" errors:
   - Wait for a minute for the OS to release the port
   - Choose a different port number
   - The dynamic TCP port allocation should minimize these issues

2. **Client can't find streaming port**: If TCP clients have trouble connecting:
   - Ensure the server is running correctly
   - The client will automatically retry port discovery several times

3. **UDP "Message too long" errors**: This is prevented by using smaller chunk sizes for UDP

4. **Connection refused**: Make sure the server is running before starting clients

5. **Memory errors**: The code uses proper memory management with checks in place

## Credits and License

This is an educational project developed for a networking course assignment. Free to use and modify for educational purposes. 