#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdbool.h>

// Match server's configuration
#define TCP_CHUNK_SIZE 131072  // 128KB for TCP
#define UDP_CHUNK_SIZE 8192    // 8KB for UDP
#define BUFFER_SIZE TCP_CHUNK_SIZE  // Use the larger size for our buffer

// Request and Response Types
#define TYPE_1_REQUEST 1  // Client request message
#define TYPE_2_RESPONSE 2  // Server response message

// Protocol modes
#define MODE_TCP 1
#define MODE_UDP 2

// Message structure for client-server communication
typedef struct {
    int type;               // Message type (1 for request, 2 for response)
    char resolution[10];    // Video resolution (480p, 720p, 1080p)
    int bandwidth;          // Estimated bandwidth in Kbps (set by server)
    char protocol[10];      // Protocol (TCP or UDP)
    int streaming_port;     // Port for streaming (assigned by server)
} Message;

// Get current time in seconds
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Send Type 1 Request and receive Type 2 Response using TCP
int connection_phase(const char *server_ip, int server_port, const char *resolution, const char *protocol, int *streaming_port) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    Message request, response;
    
    // Create TCP socket for connection phase
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    
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
    
    printf("Connected to server at %s:%d for connection phase\n", server_ip, server_port);
    
    // Prepare Type 1 Request
    request.type = TYPE_1_REQUEST;
    strncpy(request.resolution, resolution, sizeof(request.resolution));
    strncpy(request.protocol, protocol, sizeof(request.protocol));
    request.bandwidth = 0; // Client doesn't set bandwidth
    
    // Send Type 1 Request
    if (send(sock, &request, sizeof(request), 0) < 0) {
        perror("Failed to send request message");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Sent Type 1 Request with resolution: %s and protocol: %s\n", resolution, protocol);
    
    // Receive Type 2 Response
    ssize_t bytes_received = recv(sock, &response, sizeof(response), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive response from server");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    if (response.type != TYPE_2_RESPONSE) {
        printf("Received invalid response type\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Extract streaming port assigned by server
    *streaming_port = response.streaming_port;
    
    printf("Received Type 2 Response - Selected resolution: %s, Bandwidth requirement: %d Kbps, Streaming port: %d\n", 
           response.resolution, response.bandwidth, *streaming_port);
    
    // Close the connection phase socket
    close(sock);
    
    return response.bandwidth;
}

// TCP client implementation for video streaming
void tcp_client(const char *server_ip, int server_port, const char *resolution) {
    int streaming_port = server_port; // Default value
    int bandwidth = connection_phase(server_ip, server_port, resolution, "TCP", &streaming_port);
    
    // For TCP, we need to wait for the streaming port to be assigned
    if (streaming_port == 0) {
        int retry_count = 0;
        int max_retries = 5; // Maximum number of retries
        
        printf("Waiting for TCP streaming port assignment...\n");
        sleep(1); // Give the server a moment to set up
        
        while (retry_count < max_retries) {
            // Create a polling socket for port discovery
            int poll_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (poll_sock < 0) {
                perror("TCP polling socket creation failed");
                exit(EXIT_FAILURE);
            }
            
            struct sockaddr_in poll_addr;
            memset(&poll_addr, 0, sizeof(poll_addr));
            poll_addr.sin_family = AF_INET;
            poll_addr.sin_port = htons(server_port);
            
            if (inet_pton(AF_INET, server_ip, &poll_addr.sin_addr) <= 0) {
                perror("Invalid address for polling");
                close(poll_sock);
                exit(EXIT_FAILURE);
            }
            
            // Connect to server
            if (connect(poll_sock, (struct sockaddr *)&poll_addr, sizeof(poll_addr)) < 0) {
                perror("TCP polling connection failed");
                close(poll_sock);
                retry_count++;
                sleep(1);
                continue;
            }
            
            // Request the streaming port
            if (send(poll_sock, "GET_STREAM_PORT", strlen("GET_STREAM_PORT"), 0) < 0) {
                perror("Failed to request stream port");
                close(poll_sock);
                retry_count++;
                sleep(1);
                continue;
            }
            
            // Wait for server to respond with port number
            char port_buffer[32] = {0};
            if (recv(poll_sock, port_buffer, sizeof(port_buffer), 0) <= 0) {
                printf("Retry %d/%d: No port response yet\n", retry_count + 1, max_retries);
                close(poll_sock);
                retry_count++;
                sleep(1);
                continue;
            }
            
            // Extract port from response
            if (sscanf(port_buffer, "PORT:%d", &streaming_port) == 1 && streaming_port > 0) {
                printf("Received dynamic TCP streaming port: %d\n", streaming_port);
                close(poll_sock);
                break;
            } else {
                printf("Retry %d/%d: Invalid port response: %s\n", retry_count + 1, max_retries, port_buffer);
                close(poll_sock);
                retry_count++;
                sleep(1);
                continue;
            }
        }
        
        if (streaming_port <= 0) {
            printf("Failed to get streaming port after %d attempts\n", max_retries);
            exit(EXIT_FAILURE);
        }
    }
    
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Create socket for video streaming
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(streaming_port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }
    
    printf("Connecting to TCP streaming server at %s:%d...\n", server_ip, streaming_port);
    
    // Add a longer delay to allow server time to set up streaming socket
    printf("Waiting for server to be ready (2 seconds)...\n");
    sleep(2); // 2 seconds
    
    // Connect to server with retries
    int connected = 0;
    int retry_count = 0;
    int max_retries = 10;
    
    while (!connected && retry_count < max_retries) {
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
            connected = 1;
        } else {
            retry_count++;
            printf("Connection attempt %d failed, retrying in 1 second...\n", retry_count);
            sleep(1);
        }
    }
    
    if (!connected) {
        perror("TCP connection failed after multiple attempts");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to TCP server at %s:%d for video streaming\n", server_ip, streaming_port);
    
    // Wait for server to be ready
    int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0 || strcmp(buffer, "READY_TO_STREAM") != 0) {
        printf("Server not ready to stream\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Send confirmation to start streaming
    send(sock, "START_STREAM", strlen("START_STREAM"), 0);
    
    printf("Starting video stream reception (TCP, %s)...\n", resolution);
    
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
            printf("Resolution: %s (Bandwidth: %d Kbps)\n", resolution, bandwidth);
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

// UDP client implementation for video streaming
void udp_client(const char *server_ip, int server_port, const char *resolution) {
    int streaming_port = server_port; // Default value
    int bandwidth = connection_phase(server_ip, server_port, resolution, "UDP", &streaming_port);
    
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
    serv_addr.sin_port = htons(streaming_port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }
    
    printf("Requesting video stream from UDP server at %s:%d\n", server_ip, streaming_port);
    
    // Add a longer delay to allow server time to set up streaming socket
    printf("Waiting for server to be ready (2 seconds)...\n");
    sleep(2); // 2 seconds
    
    // Send request to start streaming - retry up to 3 times
    int retry_count = 0;
    int max_retries = 3;
    bool received_ready = false;
    
    while (retry_count < max_retries && !received_ready) {
        // Send request
        sendto(sock, "REQUEST_STREAM", strlen("REQUEST_STREAM"), 0, 
               (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        
        // Wait for server to be ready with a timeout
        struct timeval tv;
        tv.tv_sec = 2;  // 2 seconds timeout for initial response
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        socklen_t server_addr_len = sizeof(serv_addr);
        int bytes_received = recvfrom(sock, buffer, BUFFER_SIZE, 0, 
                                     (struct sockaddr *)&serv_addr, &server_addr_len);
        
        if (bytes_received > 0 && strcmp(buffer, "READY_TO_STREAM") == 0) {
            received_ready = true;
        } else {
            retry_count++;
            printf("Retry %d/%d: Waiting for server response...\n", retry_count, max_retries);
        }
    }
    
    if (!received_ready) {
        printf("Server not ready to stream after %d attempts\n", max_retries);
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Starting video stream reception (UDP, %s)...\n", resolution);
    
    // Set longer timeout for video streaming
    struct timeval stream_tv;
    stream_tv.tv_sec = 5;  // 5 seconds timeout
    stream_tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &stream_tv, sizeof(stream_tv));
    
    // Start receiving video
    char video_chunk[UDP_CHUNK_SIZE] = {0};
    double start_time = get_time();
    double last_stats_time = start_time;
    int chunks_received = 0;
    unsigned long total_data = 0;
    unsigned long recent_data = 0;
    int lost_packets = 0;
    int expected_chunk_id = 0;
    
    while (1) {
        memset(video_chunk, 0, UDP_CHUNK_SIZE);
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        socklen_t server_addr_len = sizeof(serv_addr);
        
        // Set socket timeout
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        // Receive video chunk
        int bytes_received = recvfrom(sock, video_chunk, UDP_CHUNK_SIZE, 0, 
                                      (struct sockaddr *)&serv_addr, &server_addr_len);
        
        if (bytes_received <= 0) {
            printf("\nTimeout or end of stream\n");
            break;
        }
        
        // Update statistics
        total_data += bytes_received;
        chunks_received++;
        double current_time = get_time();
        double elapsed = current_time - start_time;
        double interval = current_time - last_stats_time;
        
        // Check for lost packets
        int chunk_id;
        sscanf(video_chunk, "VIDEO_CHUNK_%d_", &chunk_id);
        
        if (expected_chunk_id != -1 && chunk_id != expected_chunk_id) {
            lost_packets += (chunk_id - expected_chunk_id - 1);
            printf("\nPacket loss detected! Expected %d, got %d\n", 
                   expected_chunk_id + 1, chunk_id);
        }
        expected_chunk_id = chunk_id;
        
        // Print statistics every 10 chunks
        if (chunks_received % 10 == 0) {
            double overall_data_rate = total_data / elapsed;
            if (interval > 0) {
                recent_data = (bytes_received * 10) / interval;
            }
            
            printf("\n----- UDP Streaming Statistics -----\n");
            printf("Resolution: %s (Bandwidth: %d Kbps)\n", resolution, bandwidth);
            printf("Chunks received: %d\n", chunks_received);
            printf("Total data received: %lu bytes\n", total_data);
            printf("Elapsed time: %.2f seconds\n", elapsed);
            printf("Overall data rate: %.2f bytes/sec\n", overall_data_rate);
            printf("Recent data rate: %lu bytes/sec\n", recent_data);
            printf("Lost packets: %d\n", lost_packets);
            printf("Packet loss rate: %.2f%%\n", 
                   (lost_packets * 100.0) / (chunks_received + lost_packets));
            printf("------------------------------------\n");
            
            last_stats_time = current_time;
        }
        
        // Simulate video processing
        printf("\rReceiving chunk #%d", chunk_id);
        fflush(stdout);
    }
    
    printf("\nStream ended after receiving %d chunks\n", chunks_received);
    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <Server IP> <Server Port> <Resolution: 480p/720p/1080p> <Mode: TCP/UDP>\n", argv[0]);
        return -1;
    }
    
    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    const char *resolution = argv[3];
    const char *mode = argv[4];
    
    // Validate resolution
    if (strcmp(resolution, "480p") != 0 && 
        strcmp(resolution, "720p") != 0 && 
        strcmp(resolution, "1080p") != 0) {
        printf("Invalid resolution. Use '480p', '720p', or '1080p'.\n");
        return -1;
    }
    
    // Validate mode and call appropriate client function
    if (strcasecmp(mode, "TCP") == 0) {
        tcp_client(server_ip, server_port, resolution);
    } else if (strcasecmp(mode, "UDP") == 0) {
        udp_client(server_ip, server_port, resolution);
    } else {
        printf("Invalid mode. Use 'TCP' or 'UDP'.\n");
        return -1;
    }
    
    return 0;
} 