# Video Streaming Simulation

This project implements a video streaming simulation using TCP, UDP, and multithreading as part of the CS558 socket programming assignment.

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

- GCC compiler
- POSIX-compliant operating system (Linux, macOS, etc.)
- pthread library

## Compilation

Use the provided Makefile to compile both server and client programs:

```bash
make
```

This will create two executable files:
- `server` - The video streaming server
- `client` - The client application for receiving video streams

To clean the compiled files:

```bash
make clean
```

## Running the Application

### Starting the Server

Start the server first:

```bash
./server
```

The server will start listening for TCP connections on port 8080 and UDP connections on port 8081.

### Running the TCP Client

Open a new terminal and run:

```bash
./client 127.0.0.1 tcp
```

Replace `127.0.0.1` with the server's IP address if running on a different machine.

### Running the UDP Client

Open another terminal and run:

```bash
./client 127.0.0.1 udp
```

### Viewing Server Statistics

To view the streaming statistics on the server side, press `Ctrl+C` in the server terminal. This will display statistics for all connected clients before the server exits.

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