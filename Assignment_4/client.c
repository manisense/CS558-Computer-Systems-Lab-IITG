#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define BUFFER_SIZE 2048

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
} Message;

// Get current time in seconds
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Send Type 1 Request and receive Type 2 Response using TCP
int connection_phase(const char *server_ip, int server_port, const char *resolution) {
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
    request.bandwidth = 0; // Client doesn't set bandwidth
    
    // Send Type 1 Request
    if (send(sock, &request, sizeof(request), 0) < 0) {
        perror("Failed to send request message");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Sent Type 1 Request with resolution: %s\n", resolution);
    
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
    
    printf("Received Type 2 Response - Selected resolution: %s, Bandwidth requirement: %d Kbps\n", 
           response.resolution, response.bandwidth);
    
    // Close the connection phase socket
    close(sock);
    
    return response.bandwidth;
}

// TCP client implementation for video streaming
void tcp_client(const char *server_ip, int server_port, const char *resolution) {
    int bandwidth = connection_phase(server_ip, server_port, resolution);
    
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Create socket for video streaming
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
    
    printf("Connected to TCP server at %s:%d for video streaming\n", server_ip, server_port);
    
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
    int bandwidth = connection_phase(server_ip, server_port, resolution);
    
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
    serv_addr.sin_port = htons(server_port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }
    
    printf("Requesting video stream from UDP server at %s:%d\n", server_ip, server_port);
    
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
    
    printf("Starting video stream reception (UDP, %s)...\n", resolution);
    
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
            printf("Resolution: %s (Bandwidth: %d Kbps)\n", resolution, bandwidth);
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
    if (argc != 5) {
        printf("Usage: %s <Server IP> <Server Port> <Mode: TCP/UDP> <Resolution: 480p/720p/1080p>\n", argv[0]);
        return -1;
    }
    
    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    const char *mode = argv[3];
    const char *resolution = argv[4];
    
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