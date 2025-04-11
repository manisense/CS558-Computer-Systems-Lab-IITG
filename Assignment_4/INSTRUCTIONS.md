# Video Streaming Simulation - Step-by-Step Instructions

## Overview
This document provides detailed instructions for compiling and running the video streaming simulation application. The application demonstrates TCP, UDP, and multithreading capabilities for handling concurrent video streaming clients.

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
./server 8080 FCFS
```

This starts the server:
- On port `8080`
- Using the `FCFS` (First-Come-First-Serve) scheduling policy

You should see output indicating the server has started:
```
TCP server started on port 8080 with FCFS scheduling policy
Scheduler started with FCFS policy
```

Alternatively, you can use the Round-Robin scheduling policy:
```bash
./server 8080 RR
```

Keep this terminal window open as the server must continue running for clients to connect.

## Step 3: Running TCP Clients

### First TCP Client (720p)
Open a new terminal window (don't close the server terminal) and run the client in TCP mode with 720p resolution:

```bash
./client 127.0.0.1 8080 720p TCP
```

You should see output similar to:
```
Connected to server at 127.0.0.1:8080 for connection phase
Sent Type 1 Request with resolution: 720p and protocol: TCP
Received Type 2 Response - Selected resolution: 720p, Bandwidth requirement: 3000 Kbps, Streaming port: 0
Waiting for TCP streaming port assignment...
Received dynamic TCP streaming port: 52345
Connecting to TCP streaming server at 127.0.0.1:52345...
Waiting for server to be ready (2 seconds)...
Connected to TCP server at 127.0.0.1:52345 for video streaming
Starting video stream reception (TCP, 720p)...
```

The client will begin receiving video chunks and display statistics every 10 chunks:
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

### Second TCP Client (480p)
Open a third terminal window and run another TCP client with 480p resolution:

```bash
./client 127.0.0.1 8080 480p TCP
```

This demonstrates how the server can handle multiple TCP clients simultaneously, each with their own streaming port.

## Step 4: Running a UDP Client

Open a fourth terminal window and run a client in UDP mode with 1080p resolution:

```bash
./client 127.0.0.1 8080 1080p UDP
```

You should see output similar to:
```
Connected to server at 127.0.0.1:8080 for connection phase
Sent Type 1 Request with resolution: 1080p and protocol: UDP
Received Type 2 Response - Selected resolution: 1080p, Bandwidth requirement: 6000 Kbps, Streaming port: 8080
Requesting video stream from UDP server at 127.0.0.1:8080
Waiting for server to be ready (2 seconds)...
Starting video stream reception (UDP, 1080p)...
```

The UDP client will display statistics including packet loss information:
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

## Step 5: Testing Concurrent Client Handling

If you've followed steps 1-4, you now have three clients (two TCP and one UDP) streaming simultaneously from the server. This demonstrates the concurrent handling capability of the server.

Observe the following:
1. All three clients continue to receive chunks and show progress
2. The server processes clients according to the scheduling policy you chose
3. TCP clients each get their own dedicated port for streaming
4. UDP client uses the main server port for streaming

## Step 6: Viewing Server Statistics

To see statistics from the server's perspective, press `Ctrl+C` in the server terminal. This will display information about all connected clients before the server exits:

```
Server shutting down, collecting statistics...

----- Streaming Statistics -----
Total clients connected: 3

Client 0 (127.0.0.1): TCP - 720p
  Status: Streaming
  Bytes sent: 8388608
  Chunks sent: 64
  Data rate: 224531.44 bytes/sec
  Elapsed time: 37.36 seconds

Client 1 (127.0.0.1): UDP - 1080p
  Status: Streaming
  Bytes sent: 532480
  Chunks sent: 65
  Data rate: 16268.25 bytes/sec
  Elapsed time: 32.73 seconds

Client 2 (127.0.0.1): TCP - 480p
  Status: Streaming
  Bytes sent: 3145728
  Chunks sent: 24
  Data rate: 179328.51 bytes/sec
  Elapsed time: 17.54 seconds

------------------------------

Server terminated gracefully.
```

## Step 7: Testing Different Scheduling Policies

To see the difference between scheduling policies:

1. Stop all processes by pressing `Ctrl+C` in each terminal
2. Restart the server with Round-Robin scheduling:
   ```bash
   ./server 8080 RR
   ```
3. Start multiple clients as before
4. Observe how the server handles clients in a more balanced manner, giving each client a turn rather than following a strict order of arrival

## Step 8: Simulating Network Issues

To simulate network issues and test system resilience:

### For TCP:
1. Start a TCP client and let it begin streaming
2. Terminate the client abruptly with `Ctrl+C`
3. Restart the client with the same parameters
4. Observe how the server handles the reconnection

### For UDP:
UDP naturally simulates packet loss in real networks. You can observe this in the client statistics that track lost packets.

## Step 9: Testing With Different Resolutions

Try different combinations of resolutions:
- 480p: Low resolution, 1.5 Mbps bandwidth
- 720p: Medium resolution, 3 Mbps bandwidth
- 1080p: High resolution, 6 Mbps bandwidth

Compare the performance statistics for different resolutions using the same protocol.

## Step 10: Cleanup

When you're done testing, terminate all clients and the server by pressing `Ctrl+C` in each terminal window.

To clean up the compiled files:
```bash
make clean
```

## Troubleshooting

1. **"Address already in use" error**:
   - Wait approximately 60 seconds for the OS to release the port
   - Choose a different port: `./server 8081 FCFS`
   - Check if another instance of the server is already running: `ps aux | grep server`

2. **TCP client can't find streaming port**:
   - Ensure the server is running without errors
   - The client automatically retries port discovery several times
   - Check server output for any error messages

3. **UDP "Message too long" errors**:
   - This is automatically handled by using smaller chunks for UDP (8KB)
   - If you see this error, check if UDP_CHUNK_SIZE was modified

4. **Socket timeouts**:
   - The client has timeouts configured for UDP streaming
   - If a timeout occurs, check if the server is still running or overloaded

5. **Slow performance or client blocking**:
   - Try running fewer clients simultaneously
   - Check system resources (CPU, memory) to ensure they're not exhausted

## Advanced Testing

### Testing Resource Limits

To stress test the system:
1. Start the server with `./server 8080 FCFS`
2. Run a script to start many clients simultaneously:
   ```bash
   for i in {1..5}; do
     ./client 127.0.0.1 8080 720p TCP &
   done
   ```
3. Observe server behavior under load

### Comparing TCP vs UDP Performance

To directly compare TCP and UDP:
1. Start the server
2. Run identical resolution clients with different protocols:
   ```bash
   ./client 127.0.0.1 8080 720p TCP
   ./client 127.0.0.1 8080 720p UDP
   ```
3. Compare the statistics output for data rate and reliability

## Understanding the Implementation

This implementation demonstrates:

1. **Concurrent connections** through multithreading
2. **Protocol differences** between TCP and UDP
3. **Dynamic port allocation** for TCP streaming
4. **Memory-safe implementation** with proper resource management
5. **Performance monitoring** with detailed statistics
6. **Packet loss detection** for UDP connections

Each video chunk is simulated with a delay between transmissions to represent real video streaming behavior. 