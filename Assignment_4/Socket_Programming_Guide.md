# Socket Programming Assignment Guide - Group 25

## Assignment Overview
This assignment focuses on implementing network applications using socket programming in C/C++. Based on the assignment document (CS558 - Assignment_4a: Socket Programming), Group 25 is tasked with implementing **Application #2: Video Streaming Simulation using TCP, UDP, and Multithreading**.

### Context
In previous assignments, we captured and analyzed network traffic for the Hotstar platform using Wireshark. This assignment builds on our understanding of network protocols by implementing a video streaming simulation that demonstrates different network protocols and multithreading.

## Application #2: Video Streaming Simulation

### Application Requirements

The video streaming simulation should include:

1. A server program that:
   - Supports both TCP and UDP connections
   - Handles multiple client connections concurrently using threads
   - Streams video data (simulated) to clients
   - Maintains stats on connection quality and performance

2. A client program that:
   - Connects to the server using either TCP or UDP
   - Receives and processes the video stream
   - Displays statistics about the connection

### Implementation Guide

## Step 1: Setting Up the Development Environment

Before starting, ensure you have the following:

1. A C/C++ compiler (GCC recommended)
2. Knowledge of socket programming and multithreading
3. Text editor or IDE (VSCode, Eclipse, etc.)
4. Required libraries: pthread for multithreading

```bash
# Check if GCC is installed
gcc --version

# If not installed, on Ubuntu/Debian:
sudo apt-get update
sudo apt-get install build-essential

# Make sure pthread library is available
sudo apt-get install libpthread-stubs0-dev
```

## Step 2: Implementing the Server Program

Create a file named `server.c` with the following structure:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

#define TCP_PORT 8080
#define UDP_PORT 8081
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define VIDEO_CHUNK_SIZE 1024
#define VIDEO_CHUNKS 100

// Statistics structure
typedef struct {
    int client_id;
    char protocol[10];
    struct sockaddr_in address;
    unsigned long bytes_sent;
    int chunks_sent;
    double start_time;
    double current_time;
    double data_rate;
} ClientStats;

ClientStats client_stats[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// Get current time in seconds
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Simulate video data
void generate_video_chunk(char *buffer, int chunk_id) {
    // In a real application, this would read actual video data
    // For simulation, we'll just fill the buffer with identifiable data
    sprintf(buffer, "VIDEO_CHUNK_%d_", chunk_id);
    // Fill the rest with random data to simulate video payload
    for (int i = strlen(buffer); i < VIDEO_CHUNK_SIZE - 1; i++) {
        buffer[i] = 'A' + (rand() % 26);
    }
    buffer[VIDEO_CHUNK_SIZE - 1] = '\0';
}

// Update client statistics
void update_stats(int client_id, int bytes, char *protocol) {
    pthread_mutex_lock(&stats_mutex);
    
    client_stats[client_id].bytes_sent += bytes;
    client_stats[client_id].chunks_sent++;
    client_stats[client_id].current_time = get_time();
    double elapsed = client_stats[client_id].current_time - client_stats[client_id].start_time;
    client_stats[client_id].data_rate = (elapsed > 0) ? 
                                        (client_stats[client_id].bytes_sent / elapsed) : 0;
    
    pthread_mutex_unlock(&stats_mutex);
}

// Print statistics for all clients
void print_stats() {
    pthread_mutex_lock(&stats_mutex);
    
    printf("\n----- Streaming Statistics -----\n");
    for (int i = 0; i < client_count; i++) {
        if (client_stats[i].chunks_sent > 0) {
            printf("Client %d (%s): %s\n", 
                   client_stats[i].client_id,
                   inet_ntoa(client_stats[i].address.sin_addr),
                   client_stats[i].protocol);
            printf("  Bytes sent: %lu\n", client_stats[i].bytes_sent);
            printf("  Chunks sent: %d\n", client_stats[i].chunks_sent);
            printf("  Data rate: %.2f bytes/sec\n", client_stats[i].data_rate);
            double elapsed = client_stats[i].current_time - client_stats[i].start_time;
            printf("  Elapsed time: %.2f seconds\n\n", elapsed);
        }
    }
    
    pthread_mutex_unlock(&stats_mutex);
}

// Handle TCP client in a separate thread
void *handle_tcp_client(void *arg) {
    int client_socket = *(int*)arg;
    free(arg);
    
    // Initialize client statistics
    int client_id;
    pthread_mutex_lock(&stats_mutex);
    client_id = client_count++;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(client_socket, (struct sockaddr*)&addr, &addr_size);
    
    client_stats[client_id].client_id = client_id;
    strcpy(client_stats[client_id].protocol, "TCP");
    client_stats[client_id].address = addr;
    client_stats[client_id].bytes_sent = 0;
    client_stats[client_id].chunks_sent = 0;
    client_stats[client_id].start_time = get_time();
    client_stats[client_id].current_time = client_stats[client_id].start_time;
    client_stats[client_id].data_rate = 0;
    pthread_mutex_unlock(&stats_mutex);
    
    printf("TCP client connected: %s\n", inet_ntoa(addr.sin_addr));
    
    // Send initial message to client
    char message[100];
    sprintf(message, "READY_TO_STREAM");
    send(client_socket, message, strlen(message), 0);
    
    // Receive confirmation from client
    char buffer[BUFFER_SIZE] = {0};
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0 || strcmp(buffer, "START_STREAM") != 0) {
        printf("Client did not confirm stream start\n");
        close(client_socket);
        return NULL;
    }
    
    // Start streaming video
    for (int i = 0; i < VIDEO_CHUNKS; i++) {
        char video_chunk[VIDEO_CHUNK_SIZE];
        generate_video_chunk(video_chunk, i);
        
        int bytes_sent = send(client_socket, video_chunk, VIDEO_CHUNK_SIZE, 0);
        if (bytes_sent <= 0) {
            printf("Failed to send chunk to TCP client\n");
            break;
        }
        
        update_stats(client_id, bytes_sent, "TCP");
        
        // Add a small delay to simulate video streaming
        usleep(100000);  // 100ms
    }
    
    printf("Finished streaming to TCP client %d\n", client_id);
    close(client_socket);
    return NULL;
}

// Handle UDP streaming
void *handle_udp_server(void *arg) {
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("UDP socket creation failed");
        return NULL;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(UDP_PORT);
    
    if (bind(udp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP bind failed");
        close(udp_socket);
        return NULL;
    }
    
    printf("UDP server started on port %d\n", UDP_PORT);
    
    while (1) {
        // Wait for a client request
        char buffer[BUFFER_SIZE] = {0};
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        int bytes_received = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, 
                                     (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (bytes_received <= 0) {
            continue;
        }
        
        // Check if it's a streaming request
        if (strcmp(buffer, "REQUEST_STREAM") == 0) {
            printf("UDP client requesting stream: %s\n", inet_ntoa(client_addr.sin_addr));
            
            // Initialize client statistics
            int client_id;
            pthread_mutex_lock(&stats_mutex);
            client_id = client_count++;
            
            client_stats[client_id].client_id = client_id;
            strcpy(client_stats[client_id].protocol, "UDP");
            client_stats[client_id].address = client_addr;
            client_stats[client_id].bytes_sent = 0;
            client_stats[client_id].chunks_sent = 0;
            client_stats[client_id].start_time = get_time();
            client_stats[client_id].current_time = client_stats[client_id].start_time;
            client_stats[client_id].data_rate = 0;
            pthread_mutex_unlock(&stats_mutex);
            
            // Send ready message
            char message[100];
            sprintf(message, "READY_TO_STREAM");
            sendto(udp_socket, message, strlen(message), 0, 
                   (struct sockaddr*)&client_addr, client_addr_len);
            
            // Start streaming video
            for (int i = 0; i < VIDEO_CHUNKS; i++) {
                char video_chunk[VIDEO_CHUNK_SIZE];
                generate_video_chunk(video_chunk, i);
                
                int bytes_sent = sendto(udp_socket, video_chunk, VIDEO_CHUNK_SIZE, 0, 
                                      (struct sockaddr*)&client_addr, client_addr_len);
                if (bytes_sent <= 0) {
                    printf("Failed to send chunk to UDP client\n");
                    break;
                }
                
                update_stats(client_id, bytes_sent, "UDP");
                
                // Add a small delay to simulate video streaming
                usleep(100000);  // 100ms
            }
            
            printf("Finished streaming to UDP client %d\n", client_id);
        }
    }
    
    close(udp_socket);
    return NULL;
}

// Signal handler for printing statistics
void signal_handler(int sig) {
    if (sig == SIGINT) {
        print_stats();
        exit(0);
    }
}

int main() {
    // Set up signal handler for Ctrl+C
    signal(SIGINT, signal_handler);
    
    // Create TCP socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options for TCP
    int opt = 1;
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("TCP setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Bind TCP socket
    struct sockaddr_in tcp_address;
    tcp_address.sin_family = AF_INET;
    tcp_address.sin_addr.s_addr = INADDR_ANY;
    tcp_address.sin_port = htons(TCP_PORT);
    
    if (bind(tcp_socket, (struct sockaddr *)&tcp_address, sizeof(tcp_address)) < 0) {
        perror("TCP bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for TCP connections
    if (listen(tcp_socket, 5) < 0) {
        perror("TCP listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("TCP server started on port %d\n", TCP_PORT);
    
    // Start UDP server thread
    pthread_t udp_thread;
    if (pthread_create(&udp_thread, NULL, handle_udp_server, NULL) != 0) {
        perror("Failed to create UDP server thread");
        exit(EXIT_FAILURE);
    }
    
    // Accept TCP clients and create a thread for each
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int *client_socket = malloc(sizeof(int));
        
        *client_socket = accept(tcp_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (*client_socket < 0) {
            perror("Accept failed");
            free(client_socket);
            continue;
        }
        
        // Create thread to handle client
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_tcp_client, client_socket) != 0) {
            perror("Failed to create client thread");
            close(*client_socket);
            free(client_socket);
        }
        
        // Detach the thread so it cleans up automatically
        pthread_detach(client_thread);
    }
    
    // Clean up and close sockets
    close(tcp_socket);
    return 0;
}
```

## Step 3: Implementing the Client Program

Create a file named `client.c` with the following structure:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define TCP_PORT 8080
#define UDP_PORT 8081
#define BUFFER_SIZE 2048

// Get current time in seconds
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// TCP client implementation
void tcp_client(const char *server_ip) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TCP_PORT);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("TCP connection failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to TCP server at %s:%d\n", server_ip, TCP_PORT);
    
    // Wait for server to be ready
    int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0 || strcmp(buffer, "READY_TO_STREAM") != 0) {
        printf("Server not ready to stream\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Send confirmation to start streaming
    send(sock, "START_STREAM", strlen("START_STREAM"), 0);
    
    printf("Starting video stream reception (TCP)...\n");
    
    // Statistics variables
    double start_time = get_time();
    double last_time = start_time;
    unsigned long total_bytes = 0;
    int chunks_received = 0;
    double last_data_rate = 0;
    
    // Receive video stream
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        
        if (bytes_received <= 0) {
            printf("Connection closed by server or error\n");
            break;
        }
        
        // Update statistics
        total_bytes += bytes_received;
        chunks_received++;
        double current_time = get_time();
        double elapsed = current_time - start_time;
        double interval = current_time - last_time;
        
        // Print statistics every 10 chunks
        if (chunks_received % 10 == 0) {
            double overall_data_rate = total_bytes / elapsed;
            if (interval > 0) {
                last_data_rate = (bytes_received * 10) / interval;
            }
            
            printf("\n----- TCP Streaming Statistics -----\n");
            printf("Chunks received: %d\n", chunks_received);
            printf("Total data received: %lu bytes\n", total_bytes);
            printf("Elapsed time: %.2f seconds\n", elapsed);
            printf("Overall data rate: %.2f bytes/sec\n", overall_data_rate);
            printf("Recent data rate: %.2f bytes/sec\n", last_data_rate);
            printf("------------------------------------\n");
            
            last_time = current_time;
        }
        
        // Simulate video processing (just printing the chunk ID)
        int chunk_id;
        sscanf(buffer, "VIDEO_CHUNK_%d_", &chunk_id);
        printf("\rReceiving chunk #%d", chunk_id);
        fflush(stdout);
    }
    
    printf("\nStream ended after receiving %d chunks\n", chunks_received);
    close(sock);
}

// UDP client implementation
void udp_client(const char *server_ip) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(UDP_PORT);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }
    
    printf("Requesting video stream from UDP server at %s:%d\n", server_ip, UDP_PORT);
    
    // Send request to start streaming
    sendto(sock, "REQUEST_STREAM", strlen("REQUEST_STREAM"), 0, 
           (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    // Wait for server to be ready
    socklen_t server_addr_len = sizeof(serv_addr);
    int bytes_received = recvfrom(sock, buffer, BUFFER_SIZE, 0, 
                                 (struct sockaddr *)&serv_addr, &server_addr_len);
    
    if (bytes_received <= 0 || strcmp(buffer, "READY_TO_STREAM") != 0) {
        printf("Server not ready to stream\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Starting video stream reception (UDP)...\n");
    
    // Statistics variables
    double start_time = get_time();
    double last_time = start_time;
    unsigned long total_bytes = 0;
    int chunks_received = 0;
    double last_data_rate = 0;
    int lost_packets = 0;
    int last_chunk_id = -1;
    
    // Set socket timeout to detect end of stream
    struct timeval tv;
    tv.tv_sec = 5;  // 5 seconds timeout
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Receive video stream
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recvfrom(sock, buffer, BUFFER_SIZE, 0, 
                                 (struct sockaddr *)&serv_addr, &server_addr_len);
        
        if (bytes_received <= 0) {
            printf("\nTimeout or end of stream\n");
            break;
        }
        
        // Update statistics
        total_bytes += bytes_received;
        chunks_received++;
        double current_time = get_time();
        double elapsed = current_time - start_time;
        double interval = current_time - last_time;
        
        // Check for lost packets
        int chunk_id;
        sscanf(buffer, "VIDEO_CHUNK_%d_", &chunk_id);
        
        if (last_chunk_id != -1 && chunk_id != last_chunk_id + 1) {
            lost_packets += (chunk_id - last_chunk_id - 1);
            printf("\nPacket loss detected! Expected %d, got %d\n", 
                   last_chunk_id + 1, chunk_id);
        }
        last_chunk_id = chunk_id;
        
        // Print statistics every 10 chunks
        if (chunks_received % 10 == 0) {
            double overall_data_rate = total_bytes / elapsed;
            if (interval > 0) {
                last_data_rate = (bytes_received * 10) / interval;
            }
            
            printf("\n----- UDP Streaming Statistics -----\n");
            printf("Chunks received: %d\n", chunks_received);
            printf("Total data received: %lu bytes\n", total_bytes);
            printf("Elapsed time: %.2f seconds\n", elapsed);
            printf("Overall data rate: %.2f bytes/sec\n", overall_data_rate);
            printf("Recent data rate: %.2f bytes/sec\n", last_data_rate);
            printf("Lost packets: %d\n", lost_packets);
            printf("Packet loss rate: %.2f%%\n", 
                   (lost_packets * 100.0) / (chunks_received + lost_packets));
            printf("------------------------------------\n");
            
            last_time = current_time;
        }
        
        // Simulate video processing
        printf("\rReceiving chunk #%d", chunk_id);
        fflush(stdout);
    }
    
    printf("\nStream ended after receiving %d chunks\n", chunks_received);
    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <server_ip> <protocol>\n", argv[0]);
        printf("  protocol: tcp or udp\n");
        return -1;
    }
    
    const char *server_ip = argv[1];
    const char *protocol = argv[2];
    
    if (strcmp(protocol, "tcp") == 0) {
        tcp_client(server_ip);
    } else if (strcmp(protocol, "udp") == 0) {
        udp_client(server_ip);
    } else {
        printf("Invalid protocol. Use 'tcp' or 'udp'.\n");
        return -1;
    }
    
    return 0;
}
```

## Step 4: Compilation and Execution

Compile both programs:

```bash
# Compile server with pthread library
gcc -o server server.c -lpthread

# Compile client
gcc -o client client.c
```

Run the programs:

```bash
# Start the server first
./server

# In a different terminal, run a TCP client
./client 127.0.0.1 tcp

# In another terminal, run a UDP client
./client 127.0.0.1 udp
```

## Step 5: Testing the Application

1. Start the server in one terminal:
```bash
./server
```

2. Start a TCP client in another terminal:
```bash
./client 127.0.0.1 tcp
```

3. Start a UDP client in a third terminal:
```bash
./client 127.0.0.1 udp
```

4. Observe the streaming statistics for both clients.

5. To see the server's statistics, press Ctrl+C in the server terminal window.

## Common Issues and Troubleshooting

1. **Socket creation failure**
   - Check if another program is using the same port
   - Make sure you have necessary permissions
   - Solution: Change the port numbers or run with elevated privileges

2. **Connection refused**
   - Ensure server is running before client attempts to connect
   - Check if firewall is blocking the connection
   - Solution: Disable firewall temporarily or add exception

3. **Thread creation failure**
   - Check that pthread library is properly installed and linked
   - Check for system resource limitations
   - Solution: Reduce number of threads or increase system limits

4. **UDP packet loss**
   - This is normal for UDP and part of the simulation
   - High packet loss may indicate network issues
   - Solution: Reduce network load or increase buffer sizes

5. **Performance issues**
   - Thread synchronization may cause overhead
   - Solution: Optimize mutex usage or use thread-local storage

## Performance Comparison: TCP vs UDP

Part of this assignment is to compare TCP and UDP performance for video streaming:

1. **TCP Advantages:**
   - Guaranteed delivery of all packets
   - Automatic retransmission of lost packets
   - Congestion control to avoid network overload
   - In-order delivery of packets

2. **TCP Disadvantages:**
   - Higher overhead due to connection management
   - Head-of-line blocking can cause delays
   - Less suitable for real-time applications with tight latency requirements

3. **UDP Advantages:**
   - Lower overhead (no connection management)
   - No retransmission delays
   - Better for real-time applications
   - Independent packet handling

4. **UDP Disadvantages:**
   - No guarantee of packet delivery
   - No congestion control
   - Out-of-order packet delivery possible
   - Application must handle packet loss

When running both clients, observe:
- Data rates for both protocols
- Packet loss rates (UDP only)
- Overall streaming quality and consistency

## Enhancement Ideas

Once the basic implementation is working, consider adding these enhancements:

1. **Quality of Service (QoS) simulation**
   - Dynamically adjust video quality based on network conditions
   - Implement adaptive bitrate streaming

2. **Forward Error Correction (FEC)**
   - Add redundancy to UDP packets to recover from packet loss

3. **Bandwidth throttling**
   - Simulate network congestion and observe how TCP and UDP adapt

4. **Video player simulation**
   - Add a simple buffering mechanism and playback simulation

5. **Graphical statistics**
   - Output statistics to a file and create graphs using gnuplot or similar tools

## Conclusion

This guide provides a comprehensive approach to implementing a video streaming simulation using socket programming with both TCP and UDP, along with multithreading for handling multiple clients.

By following these steps, you should be able to create a functioning simulation that demonstrates the differences between TCP and UDP protocols in the context of video streaming, which is highly relevant to your previous analysis of the Hotstar platform.

Remember to analyze the performance differences between the protocols and document your findings. Good luck with your implementation! 