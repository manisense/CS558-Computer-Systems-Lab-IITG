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
#include <ctype.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define VIDEO_CHUNK_SIZE 1024
#define VIDEO_CHUNKS 100

// Request and Response Types
#define TYPE_1_REQUEST 1  // Client request message
#define TYPE_2_RESPONSE 2  // Server response message

// Protocol modes
#define MODE_TCP 1
#define MODE_UDP 2

// Scheduling policies
#define POLICY_FCFS 1  // First-Come-First-Serve
#define POLICY_RR 2    // Round-Robin

// Connection states
#define STATE_IDLE 0
#define STATE_CONNECTION 1
#define STATE_STREAMING 2
#define STATE_FINISHED 3

// Message structure for client-server communication
typedef struct {
    int type;               // Message type (1 for request, 2 for response)
    char resolution[10];    // Video resolution (480p, 720p, 1080p)
    int bandwidth;          // Estimated bandwidth in Kbps
    char protocol[10];      // Protocol (TCP or UDP)
    int streaming_port;     // Port for streaming (assigned by server)
} Message;

// Statistics structure
typedef struct {
    int client_id;
    char protocol[10];
    char resolution[10]; // Added resolution field
    struct sockaddr_in address;
    unsigned long bytes_sent;
    int chunks_sent;
    double start_time;
    double current_time;
    double data_rate;
    int state;  // Client state
    int active; // Whether this client is active
    int streaming_port; // Port used for streaming
} ClientStats;

// Globals
ClientStats client_stats[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
int scheduling_policy = POLICY_FCFS; // Default scheduling policy
int server_port = 8080;  // Default port
int current_rr_client = 0; // For round-robin scheduling

// Queue for FCFS scheduling
typedef struct QueueNode {
    int client_id;
    struct QueueNode *next;
} QueueNode;

QueueNode *queue_head = NULL;
QueueNode *queue_tail = NULL;

// Function declarations
void *handle_connection_phase(void *arg);
void *handle_tcp_streaming(void *arg);
void *handle_udp_streaming(void *arg);
void *scheduler_thread(void *arg);
void signal_handler(int sig);
void print_stats();
void update_stats(int client_id, int bytes, const char *protocol);
void generate_video_chunk(char *buffer, int chunk_id, const char *resolution);
int estimate_bandwidth(const char *resolution);
double get_time();
int dequeue_client();
void enqueue_client(int client_id);

// Add client to queue
void enqueue_client(int client_id) {
    pthread_mutex_lock(&queue_mutex);
    
    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    new_node->client_id = client_id;
    new_node->next = NULL;
    
    if (queue_tail == NULL) {
        queue_head = new_node;
        queue_tail = new_node;
    } else {
        queue_tail->next = new_node;
        queue_tail = new_node;
    }
    
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

// Get next client from queue
int dequeue_client() {
    pthread_mutex_lock(&queue_mutex);
    
    while (queue_head == NULL) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }
    
    QueueNode *temp = queue_head;
    int client_id = temp->client_id;
    queue_head = queue_head->next;
    
    if (queue_head == NULL) {
        queue_tail = NULL;
    }
    
    free(temp);
    pthread_mutex_unlock(&queue_mutex);
    
    return client_id;
}

// Get current time in seconds
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Estimate bandwidth based on resolution
int estimate_bandwidth(const char *resolution) {
    if (strcmp(resolution, "480p") == 0) {
        return 1500;  // 1.5 Mbps for 480p
    } else if (strcmp(resolution, "720p") == 0) {
        return 3000;  // 3 Mbps for 720p
    } else if (strcmp(resolution, "1080p") == 0) {
        return 6000;  // 6 Mbps for 1080p
    } else {
        return 1000;  // Default
    }
}

// Simulate video data
void generate_video_chunk(char *buffer, int chunk_id, const char *resolution) {
    // In a real application, this would read actual video data
    // For simulation, we'll just fill the buffer with identifiable data
    sprintf(buffer, "VIDEO_CHUNK_%d_%s_", chunk_id, resolution);
    // Fill the rest with random data to simulate video payload
    for (int i = strlen(buffer); i < VIDEO_CHUNK_SIZE - 1; i++) {
        buffer[i] = 'A' + (rand() % 26);
    }
    buffer[VIDEO_CHUNK_SIZE - 1] = '\0';
}

// Update client statistics
void update_stats(int client_id, int bytes, const char *protocol) {
    pthread_mutex_lock(&stats_mutex);
    
    client_stats[client_id].bytes_sent += bytes;
    client_stats[client_id].chunks_sent++;
    client_stats[client_id].current_time = get_time();
    double elapsed = client_stats[client_id].current_time - client_stats[client_id].start_time;
    client_stats[client_id].data_rate = (elapsed > 0) ? 
                                        (client_stats[client_id].bytes_sent / elapsed) : 0;
    strncpy(client_stats[client_id].protocol, protocol, sizeof(client_stats[client_id].protocol));
    
    pthread_mutex_unlock(&stats_mutex);
}

// Print statistics for all clients
void print_stats() {
    pthread_mutex_lock(&stats_mutex);
    
    printf("\n----- Streaming Statistics -----\n");
    if (client_count == 0) {
        printf("No clients connected yet.\n");
    } else {
        // First print a summary of all clients
        printf("Total clients connected: %d\n\n", client_count);
        
        // Then print detailed stats for each client
        for (int i = 0; i < client_count; i++) {
            printf("Client %d (%s): %s - %s\n", 
                   client_stats[i].client_id,
                   inet_ntoa(client_stats[i].address.sin_addr),
                   client_stats[i].protocol,
                   client_stats[i].resolution);
                   
            if (client_stats[i].chunks_sent > 0) {
                printf("  Status: %s\n", 
                       client_stats[i].state == STATE_STREAMING ? "Streaming" : 
                       client_stats[i].state == STATE_FINISHED ? "Finished" : 
                       client_stats[i].state == STATE_IDLE ? "Idle" : "Connecting");
                printf("  Bytes sent: %lu\n", client_stats[i].bytes_sent);
                printf("  Chunks sent: %d\n", client_stats[i].chunks_sent);
                printf("  Data rate: %.2f bytes/sec\n", client_stats[i].data_rate);
                double elapsed = client_stats[i].current_time - client_stats[i].start_time;
                printf("  Elapsed time: %.2f seconds\n\n", elapsed);
            } else {
                printf("  Status: %s\n", 
                       client_stats[i].state == STATE_STREAMING ? "Streaming" : 
                       client_stats[i].state == STATE_FINISHED ? "Finished" : 
                       client_stats[i].state == STATE_IDLE ? "Idle" : "Connecting");
                printf("  No streaming data sent yet\n\n");
            }
        }
    }
    
    printf("------------------------------\n");
    pthread_mutex_unlock(&stats_mutex);
}

// Generate a random port number in the valid range
int get_random_port() {
    return 10000 + (rand() % 40000); // Between 10000 and 49999
}

// Handle the connection phase for a client
void *handle_connection_phase(void *arg) {
    int client_socket = *(int*)arg;
    free(arg);
    
    // Read the first few bytes to determine if this is a normal connection or port discovery
    char peek_buffer[16] = {0};
    int peek_result = recv(client_socket, peek_buffer, sizeof(peek_buffer) - 1, MSG_PEEK);
    
    if (peek_result > 0 && strcmp(peek_buffer, "GET_STREAM_PORT") == 0) {
        // This is a port discovery request
        // Consume the peeked data
        recv(client_socket, peek_buffer, sizeof(peek_buffer) - 1, 0);
        
        // Get client IP address
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_size);
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        printf("Port discovery request from client %s\n", client_ip);
        
        // Find the client ID based on IP address
        int found_client_id = -1;
        int found_port = -1;
        
        pthread_mutex_lock(&stats_mutex);
        printf("Searching through %d clients for port discovery\n", client_count);
        for (int i = 0; i < client_count; i++) {
            char stat_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_stats[i].address.sin_addr, stat_ip, INET_ADDRSTRLEN);
            
            printf("Client %d: IP=%s, active=%d, protocol=%s, port=%d\n", 
                   i, stat_ip, client_stats[i].active, 
                   client_stats[i].protocol, client_stats[i].streaming_port);
            
            if (strcmp(client_ip, stat_ip) == 0 && 
                client_stats[i].active && 
                strcasecmp(client_stats[i].protocol, "TCP") == 0) {
                
                found_client_id = i;
                found_port = client_stats[i].streaming_port;
                printf("Found matching client %d with port %d\n", found_client_id, found_port);
                break;
            }
        }
        pthread_mutex_unlock(&stats_mutex);
        
        // Send the port response
        char response[32];
        if (found_client_id >= 0 && found_port > 0) {
            sprintf(response, "PORT:%d", found_port);
            printf("Sending port %d to client (ID: %d)\n", found_port, found_client_id);
        } else {
            strcpy(response, "PORT:ERROR");
            printf("No port found for client %s\n", client_ip);
        }
        
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        return NULL;
    }
    
    // Register the client and get a client ID
    pthread_mutex_lock(&stats_mutex);
    int client_id = client_count++;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(client_socket, (struct sockaddr*)&addr, &addr_size);
    
    client_stats[client_id].client_id = client_id;
    client_stats[client_id].address = addr;
    client_stats[client_id].bytes_sent = 0;
    client_stats[client_id].chunks_sent = 0;
    client_stats[client_id].start_time = get_time();
    client_stats[client_id].current_time = client_stats[client_id].start_time;
    client_stats[client_id].data_rate = 0;
    client_stats[client_id].state = STATE_CONNECTION;
    client_stats[client_id].active = 1;
    pthread_mutex_unlock(&stats_mutex);
    
    printf("Client %d connected: %s\n", client_id, inet_ntoa(addr.sin_addr));
    
    // Receive Type 1 Request from client
    Message request, response;
    ssize_t bytes_received = recv(client_socket, &request, sizeof(request), 0);
    
    if (bytes_received <= 0 || request.type != TYPE_1_REQUEST) {
        printf("Invalid request from client %d\n", client_id);
        close(client_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    printf("Received Type 1 Request from client %d - Requested resolution: %s\n", 
           client_id, request.resolution);
    
    // Store the requested resolution and protocol
    pthread_mutex_lock(&stats_mutex);
    strncpy(client_stats[client_id].resolution, request.resolution, sizeof(client_stats[client_id].resolution));
    strncpy(client_stats[client_id].protocol, request.protocol, sizeof(client_stats[client_id].protocol));
    pthread_mutex_unlock(&stats_mutex);
    
    // Create Type 2 Response
    response.type = TYPE_2_RESPONSE;
    strncpy(response.resolution, request.resolution, sizeof(response.resolution));
    strncpy(response.protocol, request.protocol, sizeof(response.protocol));
    response.bandwidth = estimate_bandwidth(request.resolution);
    
    // Assign a port for streaming based on protocol
    if (strcasecmp(request.protocol, "TCP") == 0) {
        // For TCP, we'll communicate the actual port after binding
        // We'll temporarily use 0 to indicate this
        response.streaming_port = 0;
    } else {
        // For UDP, use server_port as before
        response.streaming_port = server_port;
    }
    
    // Send Type 2 Response to client
    if (send(client_socket, &response, sizeof(response), 0) < 0) {
        perror("Failed to send Type 2 Response");
        close(client_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    printf("Sent Type 2 Response to client %d - Resolution: %s, Protocol: %s, Bandwidth: %d Kbps\n", 
           client_id, response.resolution, response.protocol, response.bandwidth);
    
    // Close connection phase socket
    close(client_socket);
    
    // Update client state
    pthread_mutex_lock(&stats_mutex);
    client_stats[client_id].state = STATE_IDLE;
    pthread_mutex_unlock(&stats_mutex);
    
    // Add client to streaming queue
    enqueue_client(client_id);
    
    return NULL;
}

// Handle TCP streaming for a client
void *handle_tcp_streaming(void *arg) {
    int client_id = *(int*)arg;
    free(arg);
    
    printf("Starting TCP streaming for client %d\n", client_id);
    
    // Create a new TCP socket for streaming
    int stream_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (stream_socket < 0) {
        perror("TCP streaming socket creation failed");
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    // Enable address reuse
    int opt = 1;
    setsockopt(stream_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to a random port for TCP streaming
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(0); // Use port 0 to let the system assign a free port
    
    if (bind(stream_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed for TCP streaming socket");
        close(stream_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    // Get the assigned port number
    socklen_t len = sizeof(server_addr);
    if (getsockname(stream_socket, (struct sockaddr*)&server_addr, &len) < 0) {
        perror("getsockname failed");
        close(stream_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    int streaming_port = ntohs(server_addr.sin_port);
    
    // IMPORTANT: Update client stats with the port IMMEDIATELY 
    // so it's available for port discovery requests
    pthread_mutex_lock(&stats_mutex);
    client_stats[client_id].streaming_port = streaming_port;
    pthread_mutex_unlock(&stats_mutex);
    
    printf("TCP streaming socket bound to port %d for client %d\n", streaming_port, client_id);
    printf("Updated client %d stats with streaming port %d\n", client_id, streaming_port);
    
    // Listen for the client
    if (listen(stream_socket, 1) < 0) {
        perror("Listen failed for TCP streaming socket");
        close(stream_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    printf("Listening for TCP client %d connection on port %d...\n", client_id, streaming_port);
    
    // Add a small delay to give the client time to request port info
    usleep(100000); // 100ms
    
    // Wait for client to connect
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    int client_socket = accept(stream_socket, (struct sockaddr*)&client_addr, &addr_size);
    
    if (client_socket < 0) {
        perror("Accept failed for TCP streaming");
        close(stream_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    printf("Client %d connected for TCP streaming\n", client_id);
    
    // Update client state
    pthread_mutex_lock(&stats_mutex);
    client_stats[client_id].state = STATE_STREAMING;
    pthread_mutex_unlock(&stats_mutex);
    
    // Send ready message
    send(client_socket, "READY_TO_STREAM", strlen("READY_TO_STREAM"), 0);
    
    // Wait for start confirmation
    char buffer[BUFFER_SIZE] = {0};
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0 || strcmp(buffer, "START_STREAM") != 0) {
        printf("Client did not confirm stream start\n");
        close(client_socket);
        close(stream_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    printf("Client %d confirmed TCP stream start\n", client_id);
    
    // Get resolution from client stats
    char resolution[10];
    pthread_mutex_lock(&stats_mutex);
    strncpy(resolution, client_stats[client_id].resolution, sizeof(resolution));
    pthread_mutex_unlock(&stats_mutex);
    
    // Start streaming video
    for (int i = 0; i < VIDEO_CHUNKS; i++) {
        // Check if client is still active
        pthread_mutex_lock(&stats_mutex);
        int active = client_stats[client_id].active;
        pthread_mutex_unlock(&stats_mutex);
        
        if (!active) {
            break;
        }
        
        char video_chunk[VIDEO_CHUNK_SIZE];
        generate_video_chunk(video_chunk, i, resolution);
        
        int bytes_sent = send(client_socket, video_chunk, VIDEO_CHUNK_SIZE, 0);
        if (bytes_sent <= 0) {
            printf("Failed to send chunk to TCP client %d\n", client_id);
            break;
        }
        
        update_stats(client_id, bytes_sent, "TCP");
        
        // Add a small delay to simulate video streaming
        usleep(100000);  // 100ms
    }
    
    printf("Finished streaming to TCP client %d\n", client_id);
    pthread_mutex_lock(&stats_mutex);
    client_stats[client_id].state = STATE_FINISHED;
    client_stats[client_id].active = 0;
    pthread_mutex_unlock(&stats_mutex);
    
    close(client_socket);
    close(stream_socket);
    return NULL;
}

// Handle UDP streaming for a client
void *handle_udp_streaming(void *arg) {
    int client_id = *(int*)arg;
    free(arg);
    
    printf("Starting UDP streaming for client %d\n", client_id);
    
    // Create a UDP socket for streaming
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("UDP socket creation failed");
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    // Enable address reuse
    int opt = 1;
    setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to the server port, not a dynamic port, for UDP
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);  // Use server_port for UDP
    
    if (bind(udp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed for UDP socket");
        close(udp_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    printf("UDP streaming socket bound to port %d for client %d\n", server_port, client_id);
    
    // Wait for UDP datagram from the client
    char buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    printf("Waiting for UDP REQUEST_STREAM message from client %d...\n", client_id);
    int bytes_received = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, 
                                 (struct sockaddr*)&client_addr, &client_addr_len);
    
    if (bytes_received <= 0) {
        printf("Failed to receive data from UDP client %d\n", client_id);
        close(udp_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    printf("Received from client %d: %s\n", client_id, buffer);
    
    if (strcmp(buffer, "REQUEST_STREAM") != 0) {
        printf("Client %d did not request UDP stream (received: %s)\n", client_id, buffer);
        close(udp_socket);
        pthread_mutex_lock(&stats_mutex);
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
        pthread_mutex_unlock(&stats_mutex);
        return NULL;
    }
    
    // Update client address
    pthread_mutex_lock(&stats_mutex);
    client_stats[client_id].address = client_addr;
    client_stats[client_id].state = STATE_STREAMING;
    pthread_mutex_unlock(&stats_mutex);
    
    printf("Sending READY_TO_STREAM to UDP client %d\n", client_id);
    // Send ready message
    sendto(udp_socket, "READY_TO_STREAM", strlen("READY_TO_STREAM"), 0, 
           (struct sockaddr*)&client_addr, client_addr_len);
    
    // Get resolution from client stats
    char resolution[10];
    pthread_mutex_lock(&stats_mutex);
    strncpy(resolution, client_stats[client_id].resolution, sizeof(resolution));
    pthread_mutex_unlock(&stats_mutex);
    
    // Start streaming video
    for (int i = 0; i < VIDEO_CHUNKS; i++) {
        // Check if client is still active
        pthread_mutex_lock(&stats_mutex);
        int active = client_stats[client_id].active;
        pthread_mutex_unlock(&stats_mutex);
        
        if (!active) {
            break;
        }
        
        char video_chunk[VIDEO_CHUNK_SIZE];
        generate_video_chunk(video_chunk, i, resolution);
        
        int bytes_sent = sendto(udp_socket, video_chunk, VIDEO_CHUNK_SIZE, 0, 
                              (struct sockaddr*)&client_addr, client_addr_len);
        if (bytes_sent <= 0) {
            printf("Failed to send chunk to UDP client %d\n", client_id);
            break;
        }
        
        update_stats(client_id, bytes_sent, "UDP");
        
        // Add a small delay to simulate video streaming
        usleep(100000);  // 100ms
    }
    
    printf("Finished streaming to UDP client %d\n", client_id);
    pthread_mutex_lock(&stats_mutex);
    client_stats[client_id].state = STATE_FINISHED;
    client_stats[client_id].active = 0;
    pthread_mutex_unlock(&stats_mutex);
    
    close(udp_socket);
    return NULL;
}

// Handle the scheduler thread for all clients - implements both FCFS and RR
void *scheduler_thread(void *arg) {
    // Silence the unused parameter warning
    (void)arg;
    
    printf("Scheduler started with %s policy\n", 
           scheduling_policy == POLICY_FCFS ? "FCFS" : "Round-Robin");
    
    while (1) {
        int client_id;
        
        if (scheduling_policy == POLICY_FCFS) {
            // First-Come-First-Serve scheduling
            client_id = dequeue_client();
        } else {
            // Round-Robin scheduling
            pthread_mutex_lock(&stats_mutex);
            
            // Find the next active client in a round-robin fashion
            int start_id = current_rr_client;
            int found = 0;
            
            // Only consider clients in IDLE state and active
            do {
                current_rr_client = (current_rr_client + 1) % client_count;
                
                if (client_count > 0 && 
                    client_stats[current_rr_client].state == STATE_IDLE && 
                    client_stats[current_rr_client].active) {
                    found = 1;
                    break;
                }
            } while (current_rr_client != start_id && client_count > 0);
            
            if (found) {
                client_id = current_rr_client;
                // Mark as no longer IDLE to prevent repeated attempts
                client_stats[client_id].state = STATE_CONNECTION;
            } else {
                // If no idle client is found, wait for new clients
                pthread_mutex_unlock(&stats_mutex);
                sleep(1);
                continue;
            }
            
            pthread_mutex_unlock(&stats_mutex);
        }
        
        // Get the protocol from client stats
        char protocol[10];
        pthread_mutex_lock(&stats_mutex);
        strncpy(protocol, client_stats[client_id].protocol, sizeof(protocol));
        pthread_mutex_unlock(&stats_mutex);
        
        // Create a thread to handle the streaming for this client
        pthread_t thread_id;
        int *client_arg = malloc(sizeof(int));
        if (client_arg == NULL) {
            perror("Memory allocation failed");
            continue;
        }
        *client_arg = client_id;
        
        if (strcasecmp(protocol, "TCP") == 0) {
            pthread_create(&thread_id, NULL, handle_tcp_streaming, client_arg);
            pthread_detach(thread_id);
        } else if (strcasecmp(protocol, "UDP") == 0) {
            pthread_create(&thread_id, NULL, handle_udp_streaming, client_arg);
            pthread_detach(thread_id);
        } else {
            printf("Unknown protocol for client %d: %s\n", client_id, protocol);
            free(client_arg);
            
            // Mark client as finished if protocol is unknown
            pthread_mutex_lock(&stats_mutex);
            client_stats[client_id].state = STATE_FINISHED;
            client_stats[client_id].active = 0;
            pthread_mutex_unlock(&stats_mutex);
        }
        
        // If using FCFS, wait for this client to finish before serving the next one
        if (scheduling_policy == POLICY_FCFS) {
            pthread_mutex_lock(&stats_mutex);
            while (client_stats[client_id].state != STATE_FINISHED && 
                   client_stats[client_id].active) {
                pthread_mutex_unlock(&stats_mutex);
                sleep(1);
                pthread_mutex_lock(&stats_mutex);
            }
            pthread_mutex_unlock(&stats_mutex);
        } else {
            // For RR, add a small delay to avoid rapid successive attempts
            usleep(500000); // 500ms
        }
    }
    
    return NULL;
}

// Signal handler for printing statistics
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n\nServer shutting down, collecting statistics...\n");
        fflush(stdout);
        
        // Allow some time for any ongoing operations to complete
        sleep(1);
        
        // Print final statistics
        print_stats();
        
        printf("\nServer terminated gracefully.\n");
        fflush(stdout);
        
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    // Parse command line arguments
    if (argc != 3) {
        printf("Usage: %s <Server Port> <Scheduling Policy: FCFS/RR>\n", argv[0]);
        return -1;
    }
    
    server_port = atoi(argv[1]);
    
    // Convert scheduling policy to uppercase for case-insensitive comparison
    char policy[10];
    strncpy(policy, argv[2], sizeof(policy) - 1);
    policy[sizeof(policy) - 1] = '\0';
    
    for (int i = 0; policy[i]; i++) {
        policy[i] = toupper(policy[i]);
    }
    
    if (strcmp(policy, "FCFS") == 0) {
        scheduling_policy = POLICY_FCFS;
    } else if (strcmp(policy, "RR") == 0) {
        scheduling_policy = POLICY_RR;
    } else {
        printf("Invalid scheduling policy. Use 'FCFS' or 'RR'.\n");
        return -1;
    }
    
    // Setup signal handler for CTRL+C
    signal(SIGINT, signal_handler);
    
    // Initialize client statistics
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_stats[i].active = 0;
        client_stats[i].state = STATE_IDLE;
    }
    
    // Create a TCP socket for the connection phase
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        perror("TCP socket creation failed");
        return -1;
    }
    
    // Enable address reuse
    int opt = 1;
    setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind the TCP socket
    struct sockaddr_in tcp_address;
    memset(&tcp_address, 0, sizeof(tcp_address));
    tcp_address.sin_family = AF_INET;
    tcp_address.sin_addr.s_addr = INADDR_ANY;
    tcp_address.sin_port = htons(server_port);
    
    if (bind(tcp_socket, (struct sockaddr *)&tcp_address, sizeof(tcp_address)) < 0) {
        perror("TCP bind failed");
        return -1;
    }
    
    // Listen for incoming connections
    if (listen(tcp_socket, MAX_CLIENTS) < 0) {
        perror("TCP listen failed");
        return -1;
    }
    
    printf("TCP server started on port %d with %s scheduling policy\n", 
           server_port, scheduling_policy == POLICY_FCFS ? "FCFS" : "Round-Robin");
    
    // Start the scheduler thread
    pthread_t scheduler_id;
    pthread_create(&scheduler_id, NULL, scheduler_thread, NULL);
    
    // Main server loop - accept connections for the connection phase
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        
        // Accept connection from client
        int *client_socket = malloc(sizeof(int));
        *client_socket = accept(tcp_socket, (struct sockaddr *)&client_addr, &addr_size);
        
        if (*client_socket < 0) {
            perror("TCP accept failed");
            free(client_socket);
            continue;
        }
        
        // Create a thread to handle this client's connection phase
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_connection_phase, client_socket) != 0) {
            perror("Thread creation failed");
            close(*client_socket);
            free(client_socket);
            continue;
        }
        
        // Detach the thread so it can run independently
        pthread_detach(thread_id);
    }
    
    // This point should never be reached
    close(tcp_socket);
    return 0;
} 