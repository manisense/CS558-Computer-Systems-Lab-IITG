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