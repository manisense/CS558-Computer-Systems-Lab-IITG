# Video Streaming Simulation - Step-by-Step Instructions

## Overview
This document provides detailed instructions for compiling and running the video streaming simulation application that implements Task 2 of the socket programming assignment. The application demonstrates TCP, UDP, and multithreading capabilities.

## Prerequisites
- Linux/Unix operating system or macOS
- GCC compiler installed
- pthread library (usually included with the system)

## Step 1: Compilation
Open a terminal in the project directory and compile the application using the Makefile:

```bash
make
```

This command will compile both the server and client programs, creating two executable files:
- `server` - The video streaming server
- `client` - The client that connects to the server

If compilation is successful, you should see output similar to:
```
gcc -Wall -Wextra server.c -o server -lpthread
gcc -Wall -Wextra client.c -o client
```

## Step 2: Starting the Server
Run the server program first:

```bash
./server
```

You should see output indicating the server has started:
```
TCP server started on port 8080
UDP server started on port 8081
```

The server is now running and listening for connections on:
- TCP port: 8080
- UDP port: 8081

Keep this terminal window open as the server must continue running for clients to connect.

## Step 3: Running a TCP Client
Open a new terminal window (don't close the server terminal) and run the client in TCP mode:

```bash
./client 127.0.0.1 tcp
```

You should see output similar to:
```
Connected to TCP server at 127.0.0.1:8080
Starting video stream reception (TCP)...
Receiving chunk #0
```

As the streaming continues, you'll see the chunk counter increment and statistical information displayed every 10 chunks.

## Step 4: Running a UDP Client
Open a third terminal window and run a client in UDP mode:

```bash
./client 127.0.0.1 udp
```

You should see output similar to:
```
Requesting video stream from UDP server at 127.0.0.1:8081
Starting video stream reception (UDP)...
Receiving chunk #0
```

The UDP client will also display statistics every 10 chunks received.

## Step 5: Observing Performance Metrics
While both clients are running, observe the statistics printed to the terminals.

For the TCP client, you'll see output like:
```
----- TCP Streaming Statistics -----
Chunks received: 10
Total data received: 10240 bytes
Elapsed time: 1.03 seconds
Overall data rate: 9941.75 bytes/sec
Recent data rate: 9941.75 bytes/sec
------------------------------------
```

For the UDP client, you'll see similar statistics plus packet loss information:
```
----- UDP Streaming Statistics -----
Chunks received: 10
Total data received: 10240 bytes
Elapsed time: 1.02 seconds
Overall data rate: 10039.22 bytes/sec
Recent data rate: 10039.22 bytes/sec
Lost packets: 0
Packet loss rate: 0.00%
------------------------------------
```

If any UDP packets are lost, you'll see messages like:
```
Packet loss detected! Expected 5, got 7
```

## Step 6: Viewing Server Statistics
To see statistics from the server's perspective, press `Ctrl+C` in the server terminal. This will display information about all connected clients before the server exits:

```
----- Streaming Statistics -----
Client 0 (127.0.0.1): TCP
  Bytes sent: 102400
  Chunks sent: 100
  Data rate: 10240.00 bytes/sec
  Elapsed time: 10.00 seconds

Client 1 (127.0.0.1): UDP
  Bytes sent: 102400
  Chunks sent: 100
  Data rate: 10240.00 bytes/sec
  Elapsed time: 10.00 seconds
```

## Step 7: Analyzing Results
Compare the performance of TCP and UDP:

1. **Reliability**:
   - TCP should have delivered all chunks in sequence
   - UDP may have experienced packet loss (check "Lost packets" in the statistics)

2. **Performance**:
   - Compare the data rates between TCP and UDP
   - Note any differences in elapsed time for receiving the same number of chunks

3. **Behavior Under Load**:
   - You can simulate network congestion by running multiple clients simultaneously
   - Observe how each protocol handles the increased load

## Step 8: Cleanup
When you're done testing, press `Ctrl+C` in the client terminals to stop them.

To clean up the compiled files:
```bash
make clean
```

## Troubleshooting

1. **Connection refused error**:
   - Make sure the server is running before starting clients
   - Check if the ports are blocked by a firewall

2. **Socket creation failure**:
   - Another application might be using the same ports
   - Try changing the port numbers in the code and recompiling

3. **Thread creation failure**:
   - Ensure the pthread library is properly installed
   - Check system resources (too many threads may already be running)

4. **Compilation errors**:
   - Make sure GCC is installed
   - Ensure pthread development packages are installed

5. **"Protocol not available" error on macOS**:
   - This can occur because macOS implements socket options differently than Linux
   - The code has been modified to use only SO_REUSEADDR instead of combining it with SO_REUSEPORT
   - If you still encounter this issue, ensure you're using the latest version of the code

## Understanding the Implementation

This implementation demonstrates:

1. **Concurrent connections** through multithreading
2. **Protocol differences** between TCP and UDP
3. **Performance monitoring** with detailed statistics
4. **Packet loss detection** for UDP connections

Each video chunk is simulated with a delay of 100ms, creating a stream of approximately 10 chunks per second, similar to a 10 fps video stream.

## What to Include in Your Report

When documenting this implementation for your assignment, consider including:

1. A description of the architecture (client-server model with multithreading)
2. Performance comparison between TCP and UDP with actual metrics
3. Analysis of observed packet loss in UDP
4. Explanation of how multithreading enables concurrent client connections
5. Screenshots of the running application with statistics

This implementation fulfills the requirements for Task 2 of the socket programming assignment, demonstrating video streaming simulation using TCP, UDP, and multithreading. 