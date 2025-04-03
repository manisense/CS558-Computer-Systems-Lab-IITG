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