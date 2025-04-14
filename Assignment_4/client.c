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
#include <errno.h>

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
    int client_id;          // Client ID assigned by the server
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
    
    // Extract streaming port and client_id assigned by server
    *streaming_port = response.streaming_port;
    
    printf("Received Type 2 Response - Selected resolution: %s, Bandwidth requirement: %d Kbps, Streaming port: %d, Client ID: %d\n", 
           response.resolution, response.bandwidth, *streaming_port, response.client_id);
    
    // Close the connection phase socket
    close(sock);
    
    return response.client_id;
}

// TCP client implementation for video streaming
void tcp_client(const char *server_ip, int server_port, const char *resolution) {
    int streaming_port = server_port; // Default value
    int client_id = connection_phase(server_ip, server_port, resolution, "TCP", &streaming_port);
    int bandwidth = 1500; // Default value, will be set by the server
    
    // The server now uses the same port as specified in the Type 2 Response
    // streaming_port = server_port + 1;  // Remove this hardcoded value
    streaming_port = server_port + 1; // Hardcode to server_port+1 for TCP streaming
    
    printf("Using TCP streaming port: %d with client ID: %d\n", streaming_port, client_id);
    
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
    
    // Connect to server with retries and better error handling
    int connected = 0;
    int retry_count = 0;
    int max_retries = 15;  // Increased from 10 to 15
    
    while (!connected && retry_count < max_retries) {
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
            connected = 1;
            printf("Successfully connected to TCP streaming server on attempt %d\n", retry_count + 1);
        } else {
            retry_count++;
            printf("Connection attempt %d failed: %s, retrying in 1 second...\n", 
                   retry_count, strerror(errno));
            sleep(1);
        }
    }
    
    if (!connected) {
        perror("TCP connection failed after multiple attempts");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to TCP server at %s:%d for video streaming\n", server_ip, streaming_port);
    
    // Send client_id that was assigned by the server
    char id_buffer[32];
    snprintf(id_buffer, sizeof(id_buffer), "%d", client_id);
    
    printf("Sending client ID: %s\n", id_buffer);
    if (send(sock, id_buffer, strlen(id_buffer), 0) < 0) {
        perror("Failed to send client ID");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Wait for server to be ready with timeout
    struct timeval tv;
    tv.tv_sec = 15;  // Increased timeout to 15 seconds
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Wait for server to be ready
    printf("Waiting for server READY_TO_STREAM message (timeout: %d seconds)...\n", (int)tv.tv_sec);
    int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    
    if (bytes_received <= 0) {
        printf("Server not ready to stream (timeout after %d seconds)\n", (int)tv.tv_sec);
        printf("Error: %s\n", strerror(errno));
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Received message from server: '%s'\n", buffer);
    
    if (strcmp(buffer, "READY_TO_STREAM") != 0) {
        printf("Server sent unexpected message: '%s' instead of 'READY_TO_STREAM'\n", buffer);
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Received READY_TO_STREAM from server\n");
    
    // Send confirmation to start streaming
    printf("Sending START_STREAM confirmation to server\n");
    if (send(sock, "START_STREAM", strlen("START_STREAM"), 0) < 0) {
        perror("Failed to send START_STREAM confirmation");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Sent START_STREAM confirmation to server\n");
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
    int client_id = connection_phase(server_ip, server_port, resolution, "UDP", &streaming_port);
    int bandwidth = 6000; // Default value, will be set by the server
    
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
    
    printf("Requesting video stream from UDP server at %s:%d (Client ID: %d)\n", 
           server_ip, streaming_port, client_id);
    
    // Send request to start streaming - retry up to 5 times (increased from 3)
    int retry_count = 0;
    int max_retries = 5;
    bool received_ready = false;
    
    while (retry_count < max_retries && !received_ready) {
        // Send request
        if (sendto(sock, "REQUEST_STREAM", strlen("REQUEST_STREAM"), 0, 
               (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Failed to send UDP request");
            retry_count++;
            sleep(1);
            continue;
        }
        
        printf("Sent REQUEST_STREAM to server (attempt %d/%d)\n", retry_count + 1, max_retries);
        
        // Wait for server to be ready with a timeout
        struct timeval tv;
        tv.tv_sec = 3;  // 3 seconds timeout for initial response
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        socklen_t server_addr_len = sizeof(serv_addr);
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recvfrom(sock, buffer, BUFFER_SIZE, 0, 
                                     (struct sockaddr *)&serv_addr, &server_addr_len);
        
        if (bytes_received > 0) {
            if (strcmp(buffer, "READY_TO_STREAM") == 0) {
                received_ready = true;
                printf("Received READY_TO_STREAM response from server\n");
            } else {
                printf("Received unexpected response: '%s'\n", buffer);
                retry_count++;
            }
        } else {
            printf("Retry %d/%d: No response from server. Error: %s\n", 
                   retry_count + 1, max_retries, strerror(errno));
            retry_count++;
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