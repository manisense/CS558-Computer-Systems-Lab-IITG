# Video Streaming Simulation

This project implements a simulation of a video streaming service using both TCP and UDP protocols. The application consists of a Server and a Client that communicate in two distinct phases: Connection Phase and Video Streaming Phase.

## Overview

The application simulates video streaming over both TCP and UDP protocols, allowing for comparison of performance characteristics of both protocols. The server can handle multiple clients concurrently using threads.

## Features

- Server supports both TCP and UDP connections
- Multithreaded server to handle multiple clients simultaneously
- Statistical tracking of connection performance (data rate, bytes sent, etc.)
- Simulated video streaming with chunked data transfer
- UDP packet loss detection and reporting
- TCP reliable streaming

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

The server must be started first with the following command:

```bash
./server <Server Port> <Scheduling Policy: FCFS/RR>
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
./client <Server IP> <Server Port> <Mode: TCP/UDP> <Resolution: 480p/720p/1080p>
```

- `<Server IP>`: The IP address of the server (e.g., 127.0.0.1 for localhost)
- `<Server Port>`: The port number the server is listening on
- `<Mode>`: Either TCP or UDP (case-insensitive)
- `<Resolution>`: Video resolution, one of: 480p, 720p, or 1080p

Examples:
```bash
./client 127.0.0.1 8080 TCP 720p
./client 127.0.0.1 8080 UDP 1080p
```

## How It Works

### Connection Phase

1. The client initiates a TCP connection with the server
2. The client sends a Type 1 Request Message specifying the desired video resolution
3. The server responds with a Type 2 Response Message including information about the video stream and bandwidth requirements
4. The TCP connection for the connection phase is closed

### Video Streaming Phase

1. Depending on the client's chosen mode (TCP or UDP), a new connection is established
2. For TCP Mode: Video data is transmitted over a reliable TCP connection, ensuring zero packet loss
3. For UDP Mode: Data is sent via a UDP connection, emulating real-time streaming where packet loss may occur
4. The server handles multiple clients based on the selected scheduling policy:
   - FCFS: Clients are served in the order they arrive
   - RR: Clients are served in a cyclic manner, ensuring fairness

## Statistics

During the streaming, both client and server collect statistics:

- Client displays statistics every 10 chunks received, including data rate and packet loss (in UDP mode)
- Server displays statistics for all clients when terminated with Ctrl+C

## Cleaning Up

To remove the compiled executables:

```bash
make clean
```

## Notes for macOS Users

This implementation has been tested and works correctly on macOS, including the M2 chip architecture.

## Comparison of TCP and UDP

Running both TCP and UDP clients allows for comparison of the performance characteristics:

- TCP provides reliable, in-order delivery but with higher overhead
- UDP provides faster delivery but may experience packet loss
- The client application displays statistics that help visualize these differences

## Troubleshooting

1. **Socket creation failed**: Make sure no other application is using the same ports.
2. **Connection refused**: Ensure the server is running before starting the clients.
3. **Thread creation failure**: Check if pthread library is properly installed.

## Enhancements

Future enhancements could include:
- Quality of Service (QoS) simulation with adaptive bitrate
- Forward Error Correction for UDP
- Bandwidth throttling simulation
- Graphical statistics output 