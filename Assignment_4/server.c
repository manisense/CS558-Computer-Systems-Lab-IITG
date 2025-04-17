#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h> // For PRIu64 macro

// Platform-specific includes and definitions
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <process.h>
    // We'll use the compiler flag instead of pragma
    // #pragma comment(lib, "ws2_32.lib")
    
    typedef int socklen_t;
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define CLOSE_SOCKET(s) closesocket(s)
    #define strcasecmp _stricmp
    #define sleep(x) Sleep(x*1000)
    #define usleep(x) Sleep(x/1000)
    
    // Thread-related definitions for Windows
    typedef HANDLE thread_t;
    #define THREAD_RETURN_TYPE unsigned __stdcall
    #define THREAD_PARAM LPVOID
    #define THREAD_CREATE(thread, func, arg) \
        ((thread = (HANDLE)_beginthreadex(NULL, 0, func, arg, 0, NULL)) != NULL)
    #define THREAD_JOIN(thread) WaitForSingleObject(thread, INFINITE); CloseHandle(thread)
    #define THREAD_DETACH(thread) CloseHandle(thread)
    
    // Mutex-related definitions for Windows
    typedef CRITICAL_SECTION mutex_t;
    #define MUTEX_INIT(mutex) InitializeCriticalSection(&mutex)
    #define MUTEX_LOCK(mutex) EnterCriticalSection(&mutex)
    #define MUTEX_UNLOCK(mutex) LeaveCriticalSection(&mutex)
    #define MUTEX_DESTROY(mutex) DeleteCriticalSection(&mutex)
    
    // Condition variable for Windows
    #if (_WIN32_WINNT >= 0x0600)  // Vista or newer
        // Simplified condition variable - just use the Windows native one
        typedef CONDITION_VARIABLE win_cond_t;
        #define WIN_COND_INIT(cond) InitializeConditionVariable(&cond)
        #define WIN_COND_SIGNAL(cond) WakeConditionVariable(&cond)
        #define WIN_COND_WAIT(cond, mutex) SleepConditionVariableCS(&cond, &mutex, INFINITE)
        #define WIN_COND_DESTROY(cond) /* Windows condition variables don't need destruction */
    #else
        // Fallback implementation for older Windows - you would need to implement this
        typedef void* win_cond_t;
        // Basic implementation stubs - these would need actual implementation for older Windows
        #define WIN_COND_INIT(cond) (cond = NULL)
        #define WIN_COND_SIGNAL(cond) 
        #define WIN_COND_WAIT(cond, mutex) Sleep(50)
        #define WIN_COND_DESTROY(cond)
    #endif
    
    #define GET_ERROR() WSAGetLastError()
    
    // Signals for Windows
    #define SIGINT CTRL_C_EVENT
    #define SIGPIPE 13
    typedef BOOL (WINAPI *SignalHandlerPtr)(DWORD);
    
    // Handle fcntl (non-blocking sockets) in Windows
    #define F_GETFL 3
    #define F_SETFL 4
    #define O_NONBLOCK 1
    int fcntl(socket_t s, int cmd, long arg) {
        u_long mode = (cmd == F_SETFL && (arg & O_NONBLOCK)) ? 1 : 0;
        return ioctlsocket(s, FIONBIO, &mode);
    }
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/time.h>
    #include <signal.h>
    #include <ctype.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <pthread.h>
    
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE -1
    #define CLOSE_SOCKET(s) close(s)
    
    // Thread-related definitions for POSIX
    typedef pthread_t thread_t;
    #define THREAD_RETURN_TYPE void*
    #define THREAD_PARAM void*
    #define THREAD_CREATE(thread, func, arg) pthread_create(&thread, NULL, func, arg)
    #define THREAD_JOIN(thread) pthread_join(thread, NULL)
    #define THREAD_DETACH(thread) pthread_detach(thread)
    
    // Mutex-related definitions for POSIX
    typedef pthread_mutex_t mutex_t;
    #define MUTEX_INIT(mutex) pthread_mutex_init(&mutex, NULL)
    #define MUTEX_LOCK(mutex) pthread_mutex_lock(&mutex)
    #define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&mutex)
    #define MUTEX_DESTROY(mutex) pthread_mutex_destroy(&mutex)
    
    #define GET_ERROR() errno
#endif

#define BUFFER_SIZE 4096
#define MAX_CLIENTS 20   // Increased from 10 to 20 for more concurrent connections
#define TCP_CHUNK_SIZE 131072  // 128KB for TCP
#define UDP_CHUNK_SIZE 8192    // 8KB for UDP to avoid "message too long" error
#define VIDEO_CHUNK_SIZE TCP_CHUNK_SIZE  // Default for backward compatibility
#define VIDEO_CHUNKS 100
#define UDP_PACKET_LOSS_RATE 5  // 5% packet loss rate for UDP simulation

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
    int client_id;          // Client ID assigned by server
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
    socket_t socket_fd;      // Socket file descriptor for TCP streaming
    double latency;     // Average latency in ms
    int packets_dropped; // Number of dropped packets (UDP only)
} ClientStats;

// Queue for FCFS scheduling
typedef struct QueueNode {
    int client_id;
    struct QueueNode *next;
} QueueNode;

// Function declarations with proper return types
#ifdef _WIN32
    THREAD_RETURN_TYPE handle_connection_phase(THREAD_PARAM arg);
    THREAD_RETURN_TYPE handle_tcp_streaming(THREAD_PARAM arg);
    THREAD_RETURN_TYPE handle_udp_streaming(THREAD_PARAM arg);
    THREAD_RETURN_TYPE scheduler_thread(THREAD_PARAM arg);
    THREAD_RETURN_TYPE handle_tcp_connections(THREAD_PARAM arg);
    BOOL WINAPI signal_handler(DWORD sig);
#else
    void *handle_connection_phase(void *arg);
    void *handle_tcp_streaming(void *arg);
    void *handle_udp_streaming(void *arg);
    void *scheduler_thread(void *arg);
    void *handle_tcp_connections(void *arg);
    void signal_handler(int sig);
#endif

// Platform-independent socket error handling
void print_socket_error(const char* message) {
#ifdef _WIN32
    int error_code = WSAGetLastError();
    char error_message[256];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 error_message, sizeof(error_message), NULL);
    fprintf(stderr, "%s: %s (Error code: %d)\n", message, error_message, error_code);
#else
    perror(message);
#endif
}

// Socket info printing helpers
#ifdef _WIN32
// Socket info printing helpers for Windows
void print_socket_info(const char* prefix, socket_t socket_val) {
    printf("%s%" PRIu64 "\n", prefix, (uint64_t)socket_val);
}
#else
// Socket info printing helpers for Unix/Linux
void print_socket_info(const char* prefix, socket_t socket_val) {
    printf("%s%d\n", prefix, socket_val);
}
#endif

// Initialize the socket system (required for Windows)
int initialize_socket_system() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 0;
    }
    return 1;
#else
    return 1; // No initialization needed for Unix/Linux
#endif
}

// Cleanup socket system (required for Windows)
void cleanup_socket_system() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Get current time in seconds
double get_time() {
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
}

// Globals
ClientStats client_stats[MAX_CLIENTS];
int client_count = 0;
#ifdef _WIN32
mutex_t stats_mutex;
mutex_t queue_mutex;
mutex_t udp_mutex;
mutex_t log_mutex;
win_cond_t queue_cond_var;  // Windows-specific condition variable
#else
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t udp_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
#endif
int scheduling_policy = POLICY_FCFS; // Default scheduling policy
int server_port = 8080;  // Default port
int current_rr_client = 0; // For round-robin scheduling
socket_t udp_socket = INVALID_SOCKET_VALUE;     // Single shared UDP socket
socket_t tcp_streaming_socket = INVALID_SOCKET_VALUE; // Single TCP socket for streaming

QueueNode *queue_head = NULL;
QueueNode *queue_tail = NULL;

void log_message(const char* format, ...);
int estimate_bandwidth(const char *resolution);
void update_stats(int client_id, int bytes, const char *protocol);
void generate_video_chunk(char *buffer, int chunk_id, const char *resolution);
void print_stats();
int dequeue_client();
void enqueue_client(int client_id);

// Thread-safe logging function
void log_message(const char* format, ...) {
#ifdef _WIN32
    MUTEX_LOCK(log_mutex);
#else
    pthread_mutex_lock(&log_mutex);
#endif
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
    
#ifdef _WIN32
    MUTEX_UNLOCK(log_mutex);
#else
    pthread_mutex_unlock(&log_mutex);
#endif
}

// Add client to queue
void enqueue_client(int client_id) {
#ifdef _WIN32
    MUTEX_LOCK(queue_mutex);
#else
    pthread_mutex_lock(&queue_mutex);
#endif
    
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
    
#ifdef _WIN32
    WIN_COND_SIGNAL(queue_cond_var);
    MUTEX_UNLOCK(queue_mutex);
#else
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
#endif
}

// Get next client from queue
int dequeue_client() {
#ifdef _WIN32
    MUTEX_LOCK(queue_mutex);
    
    while (queue_head == NULL) {
        WIN_COND_WAIT(queue_cond_var, queue_mutex);
    }
#else
    pthread_mutex_lock(&queue_mutex);
    
    while (queue_head == NULL) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }
#endif
    
    QueueNode *temp = queue_head;
    int client_id = temp->client_id;
    queue_head = queue_head->next;
    
    if (queue_head == NULL) {
        queue_tail = NULL;
    }
    
    free(temp);
#ifdef _WIN32
    MUTEX_UNLOCK(queue_mutex);
#else
    pthread_mutex_unlock(&queue_mutex);
#endif
    
    return client_id;
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
    int header_len = snprintf(buffer, 100, "VIDEO_CHUNK_%d_%s_", chunk_id, resolution);
    
    // Simulate encoding time
    usleep(50000); // 50ms of "encoding time"
    
    // Fill the rest with repeating pattern to simulate video payload
    // Safer alternative to random data for large chunks
    char pattern[10] = "VIDEODATA";
    
    // Determine the appropriate chunk size based on the buffer size
    int max_chunk_size;
    if (strcasecmp(resolution, "UDP") == 0) {
        max_chunk_size = UDP_CHUNK_SIZE - 1; // Leave space for null terminator
    } else {
        max_chunk_size = VIDEO_CHUNK_SIZE - 1; // Leave space for null terminator
    }
    
    for (int i = header_len; i < max_chunk_size - 1; i += 9) {
        int space_left = max_chunk_size - i;
        int copy_size = (space_left < 9) ? space_left : 9;
        memcpy(buffer + i, pattern, copy_size);
    }
    
    // Ensure null termination
    buffer[max_chunk_size] = '\0';
}

// Update client statistics
void update_stats(int client_id, int bytes, const char *protocol) {
#ifdef _WIN32
    MUTEX_LOCK(stats_mutex);
#else
    pthread_mutex_lock(&stats_mutex);
#endif
    
    client_stats[client_id].bytes_sent += bytes;
    client_stats[client_id].chunks_sent++;
    client_stats[client_id].current_time = get_time();
    double elapsed = client_stats[client_id].current_time - client_stats[client_id].start_time;
    client_stats[client_id].data_rate = (elapsed > 0) ? 
                                        (client_stats[client_id].bytes_sent / elapsed) : 0;
    strncpy(client_stats[client_id].protocol, protocol, sizeof(client_stats[client_id].protocol));
    client_stats[client_id].protocol[sizeof(client_stats[client_id].protocol) - 1] = '\0';
    
#ifdef _WIN32
    MUTEX_UNLOCK(stats_mutex);
#else
    pthread_mutex_unlock(&stats_mutex);
#endif
}

// Print statistics for all clients
void print_stats() {
#ifdef _WIN32
    MUTEX_LOCK(stats_mutex);
#else
    pthread_mutex_lock(&stats_mutex);
#endif
    
    log_message("\n----- Streaming Statistics -----");
    if (client_count == 0) {
        log_message("No clients connected yet.");
    } else {
        // First print a summary of all clients
        log_message("Total clients connected: %d\n", client_count);
        
        // Then print detailed stats for each client
        for (int i = 0; i < client_count; i++) {
            log_message("Client %d (%s): %s - %s", 
                   client_stats[i].client_id,
                   inet_ntoa(client_stats[i].address.sin_addr),
                   client_stats[i].protocol,
                   client_stats[i].resolution);
                   
            if (client_stats[i].chunks_sent > 0) {
                log_message("  Status: %s", 
                       client_stats[i].state == STATE_STREAMING ? "Streaming" : 
                       client_stats[i].state == STATE_FINISHED ? "Finished" : 
                       client_stats[i].state == STATE_IDLE ? "Idle" : "Connecting");
                log_message("  Bytes sent: %lu", client_stats[i].bytes_sent);
                log_message("  Chunks sent: %d", client_stats[i].chunks_sent);
                log_message("  Data rate: %.2f bytes/sec", client_stats[i].data_rate);
                double elapsed = client_stats[i].current_time - client_stats[i].start_time;
                log_message("  Elapsed time: %.2f seconds", elapsed);
                
                // Print protocol-specific metrics
                if (strcmp(client_stats[i].protocol, "UDP") == 0) {
                    log_message("  Packets dropped: %d", client_stats[i].packets_dropped);
                    float loss_rate = (client_stats[i].packets_dropped * 100.0f) / 
                                     (client_stats[i].chunks_sent + client_stats[i].packets_dropped);
                    log_message("  Packet loss rate: %.2f%%", loss_rate);
                }
                
                // Print latency if measured
                if (client_stats[i].latency > 0) {
                    log_message("  Average latency: %.2f ms", client_stats[i].latency);
                }
                
                printf("\n");
            } else {
                log_message("  Status: %s", 
                       client_stats[i].state == STATE_STREAMING ? "Streaming" : 
                       client_stats[i].state == STATE_FINISHED ? "Finished" : 
                       client_stats[i].state == STATE_IDLE ? "Idle" : "Connecting");
                log_message("  No streaming data sent yet\n");
            }
        }
    }
    
    log_message("------------------------------");
#ifdef _WIN32
    MUTEX_UNLOCK(stats_mutex);
#else
    pthread_mutex_unlock(&stats_mutex);
#endif
}

// Generate a random port number in the valid range
int get_random_port() {
    return 10000 + (rand() % 40000); // Between 10000 and 49999
}

// Handle connection phase for a new client
THREAD_RETURN_TYPE handle_connection_phase(THREAD_PARAM arg) {
    socket_t client_socket = *((socket_t *)arg);
    free(arg);
    
    // Get client's address information
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    
    // Register the client
    int client_id;
    
#ifdef _WIN32
    MUTEX_LOCK(stats_mutex);
#else
    pthread_mutex_lock(&stats_mutex);
#endif
    
    // Find a free slot or reuse an existing inactive client with the same IP
    client_id = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        // Check if this is an existing client from the same IP that's inactive
        if (!client_stats[i].active) {
            char stored_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_stats[i].address.sin_addr, stored_ip, INET_ADDRSTRLEN);
            
            // Either a new slot or an old inactive client from same IP
            if (client_id == -1 || strcmp(stored_ip, client_ip) == 0) {
                client_id = i;
                if (strcmp(stored_ip, client_ip) == 0) {
                    // Prefer reusing slots from the same client
                    break;
                }
            }
        }
    }
    
    if (client_id == -1) {
        // All client slots are occupied
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
        printf("Maximum client limit reached, rejecting new connection from %s\n", client_ip);
        CLOSE_SOCKET(client_socket);
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    // Initialize or reset client stats
    client_stats[client_id].client_id = client_id;
    client_stats[client_id].address = client_addr;
    client_stats[client_id].bytes_sent = 0;
    client_stats[client_id].chunks_sent = 0;
    client_stats[client_id].start_time = get_time();
    client_stats[client_id].current_time = client_stats[client_id].start_time;
    client_stats[client_id].data_rate = 0;
    client_stats[client_id].state = STATE_CONNECTION;
    client_stats[client_id].active = 1;
    client_stats[client_id].streaming_port = 0;
    client_stats[client_id].socket_fd = INVALID_SOCKET_VALUE;
    
    if (client_id >= client_count) {
        client_count = client_id + 1;
    }
    
#ifdef _WIN32
    MUTEX_UNLOCK(stats_mutex);
#else
    pthread_mutex_unlock(&stats_mutex);
#endif
    
    printf("Client %d connected: %s\n", client_id, client_ip);
    
    // Handle Type 1 Request
    Message request, response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    // Receive Type 1 Request with timeout
#ifdef _WIN32
    DWORD tv_ms = 5000;  // 5 seconds timeout
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_ms, sizeof(tv_ms));
#else
    struct timeval tv;
    tv.tv_sec = 5;  // 5 seconds timeout
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif
    
    int bytes_received = recv(client_socket, (char*)&request, sizeof(request), 0);
    if (bytes_received <= 0) {
        printf("Failed to receive Type 1 Request from client %d\n", client_id);
        print_socket_error("Receive error");
        CLOSE_SOCKET(client_socket);
        
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        client_stats[client_id].active = 0;
        client_stats[client_id].state = STATE_IDLE;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
        
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    if (request.type != TYPE_1_REQUEST) {
        printf("Received invalid request type from client %d\n", client_id);
        CLOSE_SOCKET(client_socket);
        
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        client_stats[client_id].active = 0;
        client_stats[client_id].state = STATE_IDLE;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
        
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    printf("Received Type 1 Request from client %d - Requested resolution: %s\n", 
           client_id, request.resolution);
    
    // Update client stats with resolution and protocol
#ifdef _WIN32
    MUTEX_LOCK(stats_mutex);
#else
    pthread_mutex_lock(&stats_mutex);
#endif
    strncpy(client_stats[client_id].resolution, request.resolution, sizeof(client_stats[client_id].resolution) - 1);
    client_stats[client_id].resolution[sizeof(client_stats[client_id].resolution) - 1] = '\0';
    strncpy(client_stats[client_id].protocol, request.protocol, sizeof(client_stats[client_id].protocol) - 1);
    client_stats[client_id].protocol[sizeof(client_stats[client_id].protocol) - 1] = '\0';
#ifdef _WIN32
    MUTEX_UNLOCK(stats_mutex);
#else
    pthread_mutex_unlock(&stats_mutex);
#endif
    
    // Prepare Type 2 Response
    response.type = TYPE_2_RESPONSE;
    strncpy(response.resolution, request.resolution, sizeof(response.resolution));
    strncpy(response.protocol, request.protocol, sizeof(response.protocol));
    response.bandwidth = estimate_bandwidth(request.resolution);
    response.client_id = client_id;  // Include client ID in response
    
    // Both TCP and UDP will use server_port
    response.streaming_port = server_port;
    
    // Send Type 2 Response
    int bytes_sent = send(client_socket, (char*)&response, sizeof(response), 0);
    if (bytes_sent <= 0) {
        printf("Failed to send Type 2 Response to client %d\n", client_id);
        CLOSE_SOCKET(client_socket);
        
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        client_stats[client_id].active = 0;
        client_stats[client_id].state = STATE_IDLE;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
        
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    printf("Sent Type 2 Response to client %d - Resolution: %s, Protocol: %s, Bandwidth: %d Kbps\n",
           client_id, response.resolution, response.protocol, response.bandwidth);
    
    // Close the connection phase socket
    CLOSE_SOCKET(client_socket);
    
    // Add the client to the queue for scheduling
    enqueue_client(client_id);
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// Handle TCP streaming for a client
THREAD_RETURN_TYPE handle_tcp_streaming(THREAD_PARAM arg) {
    int client_id = *((int *)arg);
    free(arg);
    
#ifdef _WIN32
    MUTEX_LOCK(stats_mutex);
#else
    pthread_mutex_lock(&stats_mutex);
#endif
    client_stats[client_id].state = STATE_STREAMING;
    client_stats[client_id].start_time = get_time();
    char resolution[10];
    strncpy(resolution, client_stats[client_id].resolution, sizeof(resolution));
    resolution[sizeof(resolution) - 1] = '\0'; // Ensure null termination
    socket_t client_socket = client_stats[client_id].socket_fd;
#ifdef _WIN32
    MUTEX_UNLOCK(stats_mutex);
#else
    pthread_mutex_unlock(&stats_mutex);
#endif
    
#ifdef _WIN32
    printf("*** Starting TCP streaming for client %d (socket_fd: %" PRIu64 ") ***\n", 
           client_id, (uint64_t)client_socket);
#else
    printf("*** Starting TCP streaming for client %d (socket_fd: %d) ***\n", 
           client_id, client_socket);
#endif
    
    // Double check that we have a valid socket
    if (client_socket == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        printf("*** CRITICAL ERROR: Invalid socket file descriptor (%" PRIu64 ") for client %d ***\n", 
               (uint64_t)client_socket, client_id);
#else
        printf("*** CRITICAL ERROR: Invalid socket file descriptor (%d) for client %d ***\n", 
               client_socket, client_id);
#endif
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    // Set send timeout to avoid blocking indefinitely
#ifdef _WIN32
    DWORD send_timeout = 2000;  // 2 seconds in milliseconds
    if (setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_timeout, sizeof(send_timeout)) < 0) {
#else
    struct timeval send_tv;
    send_tv.tv_sec = 2;  // Reduced from 3 to 2 seconds timeout
    send_tv.tv_usec = 0;
    if (setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_tv, sizeof(send_tv)) < 0) {
#endif
        printf("*** WARNING: Failed to set TCP send timeout for client %d ***\n", client_id);
        print_socket_error("setsockopt failed");
    }
    
    // Set socket to non-blocking mode
    int flags = fcntl(client_socket, F_GETFL, 0);
    if (flags < 0) {
        printf("*** WARNING: Failed to get socket flags for client %d ***\n", client_id);
        print_socket_error("fcntl failed");
    } else if (fcntl(client_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        printf("*** WARNING: Failed to set socket non-blocking for client %d ***\n", client_id);
        print_socket_error("fcntl failed");
    }
    
    // Send ready message
    int retry_count = 0;
    int max_retries = 5;
    bool ready_sent = false;
    
    printf("*** Attempting to send READY_TO_STREAM to client %d ***\n", client_id);
    
    while (!ready_sent && retry_count < max_retries) {
        int send_result = send(client_socket, "READY_TO_STREAM", strlen("READY_TO_STREAM"), 0);
        if (send_result < 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
#endif
                // Socket would block, retry after a short delay
                usleep(100000); // 100ms
                retry_count++;
                printf("*** TCP send would block for client %d, retry %d/%d ***\n", 
                       client_id, retry_count, max_retries);
            } else {
                printf("*** ERROR: Failed to send READY_TO_STREAM message to client %d ***\n", client_id);
                print_socket_error("Send error");
                CLOSE_SOCKET(client_socket);
#ifdef _WIN32
                MUTEX_LOCK(stats_mutex);
#else
                pthread_mutex_lock(&stats_mutex);
#endif
                client_stats[client_id].state = STATE_FINISHED;
                client_stats[client_id].active = 0;
#ifdef _WIN32
                MUTEX_UNLOCK(stats_mutex);
#else
                pthread_mutex_unlock(&stats_mutex);
#endif
#ifdef _WIN32
                return 0;
#else
                return NULL;
#endif
            }
        } else {
            printf("*** Successfully sent READY_TO_STREAM to client %d ***\n", client_id);
            ready_sent = true;
        }
    }
    
    if (!ready_sent) {
        printf("*** ERROR: Failed to send READY_TO_STREAM after %d attempts for client %d ***\n", 
               max_retries, client_id);
        CLOSE_SOCKET(client_socket);
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    printf("*** Sent READY_TO_STREAM to TCP client %d ***\n", client_id);
    
    // Wait for start confirmation with timeout
#ifdef _WIN32
    DWORD recv_timeout = 5000;  // 5 seconds in milliseconds
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_timeout, sizeof(recv_timeout)) < 0) {
#else
    struct timeval recv_tv;
    recv_tv.tv_sec = 5;  // 5 seconds timeout
    recv_tv.tv_usec = 0;
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_tv, sizeof(recv_tv)) < 0) {
#endif
        printf("*** WARNING: Failed to set TCP receive timeout for client %d ***\n", client_id);
        print_socket_error("setsockopt failed");
    }
    
    char buffer[BUFFER_SIZE] = {0};
    
    printf("*** Waiting for START_STREAM from client %d (timeout: 5 seconds) ***\n", client_id);
    
    // Using select() to properly handle non-blocking sockets with timeout
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_socket, &read_fds);
    
    // Wait up to 5 seconds for start confirmation
#ifdef _WIN32
    struct timeval select_tv;
    select_tv.tv_sec = 5;
    select_tv.tv_usec = 0;
    int select_result = select(0, &read_fds, NULL, NULL, &select_tv);
#else
    struct timeval select_tv;
    select_tv.tv_sec = 5;
    select_tv.tv_usec = 0;
    int select_result = select(client_socket + 1, &read_fds, NULL, NULL, &select_tv);
#endif
    
    if (select_result <= 0) {
        // Timeout or error
        printf("*** ERROR: Client %d did not confirm stream start (timeout or error) ***\n", client_id);
        print_socket_error("Select error");
        CLOSE_SOCKET(client_socket);
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    printf("*** Select() indicates client %d is ready to receive ***\n", client_id);
    
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        printf("*** ERROR: Client %d did not confirm stream start (error) ***\n", client_id);
        print_socket_error("Receive error");
        CLOSE_SOCKET(client_socket);
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    printf("*** Received '%s' from client %d ***\n", buffer, client_id);
    
    if (strcmp(buffer, "START_STREAM") != 0) {
        printf("*** ERROR: Client %d sent incorrect confirmation: '%s' ***\n", client_id, buffer);
        CLOSE_SOCKET(client_socket);
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    printf("*** Client %d confirmed TCP stream start ***\n", client_id);
    
    // Tracking variables for latency calculation
    double total_latency = 0.0;
    int measured_chunks = 0;
    
    // Stream video data
    char video_chunk[TCP_CHUNK_SIZE];
    for (int i = 1; i <= VIDEO_CHUNKS; i++) {
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        int active = client_stats[client_id].active;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
        
        if (!active) {
            break;
        }
        
        // Generate a chunk of video data
        generate_video_chunk(video_chunk, i, resolution);
        
        // Measure latency
        double send_time = get_time();
        
        // Send the chunk with improved error handling for non-blocking socket
        int total_sent = 0;
        int remaining = TCP_CHUNK_SIZE;
        int retry_count = 0;
        int max_send_retries = 10;
        
        while (total_sent < TCP_CHUNK_SIZE && retry_count < max_send_retries) {
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(client_socket, &write_fds);
            
            // Set timeout for select
            struct timeval write_tv;
            write_tv.tv_sec = 1;  // 1 second timeout
            write_tv.tv_usec = 0;
            
#ifdef _WIN32
            int select_result = select(0, NULL, &write_fds, NULL, &write_tv);
#else
            int select_result = select(client_socket + 1, NULL, &write_fds, NULL, &write_tv);
#endif
            
            if (select_result <= 0) {
                // Timeout or error occurred
                retry_count++;
                continue;
            }
            
            // Socket is ready for writing
            int bytes_sent = send(client_socket, video_chunk + total_sent, remaining, 0);
            
            if (bytes_sent < 0) {
#ifdef _WIN32
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
#endif
                    // Would block, try again
                    retry_count++;
                    usleep(50000); // 50ms delay before retry
                } else {
                    // Other error
                    log_message("Failed to send TCP chunk to client %d", client_id);
                    print_socket_error("Send error");
                    CLOSE_SOCKET(client_socket);
#ifdef _WIN32
                    MUTEX_LOCK(stats_mutex);
#else
                    pthread_mutex_lock(&stats_mutex);
#endif
                    client_stats[client_id].state = STATE_FINISHED;
                    client_stats[client_id].active = 0;
#ifdef _WIN32
                    MUTEX_UNLOCK(stats_mutex);
#else
                    pthread_mutex_unlock(&stats_mutex);
#endif
#ifdef _WIN32
                    return 0;
#else
                    return NULL;
#endif
                }
            } else {
                total_sent += bytes_sent;
                remaining -= bytes_sent;
                retry_count = 0; // Reset retry counter on successful send
            }
        }
        
        if (total_sent < TCP_CHUNK_SIZE) {
            log_message("Failed to send complete chunk %d to client %d after %d retries", 
                   i, client_id, max_send_retries);
            break;
        }
        
        // Calculate latency (in a real system we'd get ACKs)
        double latency_ms = (get_time() - send_time) * 1000.0; // Convert to ms
        total_latency += latency_ms;
        measured_chunks++;
        
        // Update average latency
        if (measured_chunks > 0) {
#ifdef _WIN32
            MUTEX_LOCK(stats_mutex);
#else
            pthread_mutex_lock(&stats_mutex);
#endif
            client_stats[client_id].latency = total_latency / measured_chunks;
#ifdef _WIN32
            MUTEX_UNLOCK(stats_mutex);
#else
            pthread_mutex_unlock(&stats_mutex);
#endif
        }
        
        log_message("Sent chunk %d/%d to TCP client %d", i, VIDEO_CHUNKS, client_id);
        update_stats(client_id, total_sent, "TCP");
        
        // Simulate bandwidth limitations but don't delay too much
        int bandwidth = estimate_bandwidth(resolution);
        int delay_ms = (TCP_CHUNK_SIZE * 8) / bandwidth; // time in ms to send this chunk at specified bandwidth
        delay_ms = delay_ms > 500 ? 500 : delay_ms; // Cap at 500ms max delay
        usleep(delay_ms * 1000);
    }
    
    log_message("TCP streaming completed for client %d", client_id);
    
    CLOSE_SOCKET(client_socket);
    
#ifdef _WIN32
    MUTEX_LOCK(stats_mutex);
#else
    pthread_mutex_lock(&stats_mutex);
#endif
    client_stats[client_id].state = STATE_FINISHED;
    client_stats[client_id].active = 0;
    client_stats[client_id].socket_fd = INVALID_SOCKET_VALUE; // Reset socket_fd
#ifdef _WIN32
    MUTEX_UNLOCK(stats_mutex);
#else
    pthread_mutex_unlock(&stats_mutex);
#endif
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// Handle UDP streaming for a client
THREAD_RETURN_TYPE handle_udp_streaming(THREAD_PARAM arg) {
    int client_id = *((int *)arg);
    free(arg);
    
#ifdef _WIN32
    MUTEX_LOCK(stats_mutex);
#else
    pthread_mutex_lock(&stats_mutex);
#endif
    client_stats[client_id].state = STATE_STREAMING;
    client_stats[client_id].start_time = get_time();
    client_stats[client_id].packets_dropped = 0;
    struct sockaddr_in client_addr = client_stats[client_id].address;
    char resolution[10];
    strncpy(resolution, client_stats[client_id].resolution, sizeof(resolution));
#ifdef _WIN32
    MUTEX_UNLOCK(stats_mutex);
#else
    pthread_mutex_unlock(&stats_mutex);
#endif
    
    log_message("Starting UDP streaming for client %d", client_id);

    // We'll use the shared UDP socket that's already bound to the server port

    log_message("UDP streaming thread using shared socket on port %d for client %d", 
           server_port, client_id);
    
    // Wait for client to send a message to initiate streaming
    char buffer[BUFFER_SIZE] = {0};
    log_message("Waiting for UDP REQUEST_STREAM message from client %d...", client_id);
    
    int retries = 0;
    int max_retries = 5;
    bool got_request = false;
    
    // Process incoming UDP messages to find our client
    while (retries < max_retries && !got_request) {
#ifdef _WIN32
        MUTEX_LOCK(udp_mutex);
#else
        pthread_mutex_lock(&udp_mutex);
#endif
        memset(buffer, 0, BUFFER_SIZE);
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        
        // Set a timeout for recvfrom to avoid blocking indefinitely
#ifdef _WIN32
        DWORD tv_ms = 1000;  // 1 second timeout
        setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_ms, sizeof(tv_ms));
#else
        struct timeval tv;
        tv.tv_sec = 1;  // 1 second timeout
        tv.tv_usec = 0;
        setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif
        
        int bytes_received = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0,
                                      (struct sockaddr *)&sender_addr, &sender_len);
        
        if (bytes_received > 0) {
            // Check if the sender is our client
            char sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(sender_addr.sin_addr), sender_ip, INET_ADDRSTRLEN);
            
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            
            if (strcmp(sender_ip, client_ip) == 0 && 
                strcmp(buffer, "REQUEST_STREAM") == 0) {
                log_message("Received from client %d: REQUEST_STREAM", client_id);
                got_request = true;
                
                // Store the client's UDP port which can be different from the TCP port
#ifdef _WIN32
                MUTEX_LOCK(stats_mutex);
#else
                pthread_mutex_lock(&stats_mutex);
#endif
                client_stats[client_id].address = sender_addr;
#ifdef _WIN32
                MUTEX_UNLOCK(stats_mutex);
#else
                pthread_mutex_unlock(&stats_mutex);
#endif
                
                // Send ready message
                sendto(udp_socket, "READY_TO_STREAM", strlen("READY_TO_STREAM"), 0,
                       (struct sockaddr *)&sender_addr, sender_len);
                log_message("Sending READY_TO_STREAM to UDP client %d", client_id);
            }
        } else if (bytes_received < 0) {
#ifdef _WIN32
            if (WSAGetLastError() != WSAETIMEDOUT) {
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
#endif
                print_socket_error("UDP recvfrom error");
#ifdef _WIN32
                MUTEX_UNLOCK(udp_mutex);
#else
                pthread_mutex_unlock(&udp_mutex);
#endif
                break;
            }
        }
        
#ifdef _WIN32
        MUTEX_UNLOCK(udp_mutex);
#else
        pthread_mutex_unlock(&udp_mutex);
#endif
        
        if (!got_request) {
            retries++;
            usleep(500000); // 500ms
        }
    }
    
    if (!got_request) {
        log_message("UDP client %d did not request streaming after %d attempts", 
               client_id, max_retries);
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        client_stats[client_id].state = STATE_FINISHED;
        client_stats[client_id].active = 0;
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    
    // Send video chunks
    char video_chunk[UDP_CHUNK_SIZE];
    memset(video_chunk, 0, UDP_CHUNK_SIZE);
    
    // Initialize random seed for packet loss simulation
    srand((unsigned int)time(NULL) + client_id);
    
    // Tracking variables for latency calculation
    double total_latency = 0.0;
    int measured_chunks = 0;
    
    for (int i = 1; i <= VIDEO_CHUNKS; i++) {
        // Generate a chunk of video data specifically for UDP
        int header_len = snprintf(video_chunk, 100, "VIDEO_CHUNK_%d_%s_", i, resolution);
        
        // Fill the rest with pattern data up to UDP_CHUNK_SIZE
        char pattern[10] = "VIDEODATA";
        for (int j = header_len; j < UDP_CHUNK_SIZE - 1; j += 9) {
            int space_left = UDP_CHUNK_SIZE - j - 1;
            int copy_size = (space_left < 9) ? space_left : 9;
            memcpy(video_chunk + j, pattern, copy_size);
        }
        
        // Ensure null termination
        video_chunk[UDP_CHUNK_SIZE - 1] = '\0';
        
        // Simulate random packet loss for UDP
        if (rand() % 100 < UDP_PACKET_LOSS_RATE) {
            log_message("Simulating packet loss for chunk %d to UDP client %d", i, client_id);
            
#ifdef _WIN32
            MUTEX_LOCK(stats_mutex);
#else
            pthread_mutex_lock(&stats_mutex);
#endif
            client_stats[client_id].packets_dropped++;
#ifdef _WIN32
            MUTEX_UNLOCK(stats_mutex);
#else
            pthread_mutex_unlock(&stats_mutex);
#endif
            
            // Skip sending but still track stats
            update_stats(client_id, 0, "UDP");
            continue;
        }
        
        // Send chunk to client
#ifdef _WIN32
        MUTEX_LOCK(udp_mutex);
#else
        pthread_mutex_lock(&udp_mutex);
#endif
        socklen_t client_len = sizeof(struct sockaddr_in);
        
        // Measure latency
        double send_time = get_time();
        
        int send_result = sendto(udp_socket, video_chunk, UDP_CHUNK_SIZE, 0,
               (struct sockaddr *)&client_stats[client_id].address, client_len);
        
        if (send_result < 0) {
            print_socket_error("UDP sendto error");
        } else {
            // Simulate network latency measurement (in a real system, we'd get ACKs)
            double latency_ms = (get_time() - send_time) * 1000.0; // Convert to ms
            total_latency += latency_ms;
            measured_chunks++;
            
            // Update average latency
            if (measured_chunks > 0) {
#ifdef _WIN32
                MUTEX_LOCK(stats_mutex);
#else
                pthread_mutex_lock(&stats_mutex);
#endif
                client_stats[client_id].latency = total_latency / measured_chunks;
#ifdef _WIN32
                MUTEX_UNLOCK(stats_mutex);
#else
                pthread_mutex_unlock(&stats_mutex);
#endif
            }
        }
#ifdef _WIN32
        MUTEX_UNLOCK(udp_mutex);
#else
        pthread_mutex_unlock(&udp_mutex);
#endif
        
        log_message("Sent chunk %d/%d to UDP client %d", i, VIDEO_CHUNKS, client_id);
        
        // Update statistics
        update_stats(client_id, UDP_CHUNK_SIZE, "UDP");
        
        // Simulate bandwidth limitations
        int bandwidth = estimate_bandwidth(resolution);
        int delay_ms = (UDP_CHUNK_SIZE * 8) / bandwidth; // time in ms to send this chunk at specified bandwidth
        usleep(delay_ms * 1000);
    }
    
    log_message("UDP streaming completed for client %d", client_id);
    
    // Mark client as finished
#ifdef _WIN32
    MUTEX_LOCK(stats_mutex);
#else
    pthread_mutex_lock(&stats_mutex);
#endif
    client_stats[client_id].state = STATE_FINISHED;
    client_stats[client_id].active = 0;
#ifdef _WIN32
    MUTEX_UNLOCK(stats_mutex);
#else
    pthread_mutex_unlock(&stats_mutex);
#endif
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// Scheduler thread that manages streaming for all clients
THREAD_RETURN_TYPE scheduler_thread(THREAD_PARAM arg) {
    // Silence the unused parameter warning
    (void)arg;
    
    printf("Scheduler started with %s policy\n", 
           scheduling_policy == POLICY_FCFS ? "FCFS" : "Round-Robin");
    
    while (1) {
        int client_id;
        bool client_found = false;
        
        if (scheduling_policy == POLICY_FCFS) {
            // First-Come-First-Serve scheduling
#ifdef _WIN32
            MUTEX_LOCK(queue_mutex);
#else
            pthread_mutex_lock(&queue_mutex);
#endif
            if (queue_head != NULL) {
                // Get client from queue without waiting
                QueueNode *temp = queue_head;
                client_id = temp->client_id;
                queue_head = queue_head->next;
                
                if (queue_head == NULL) {
                    queue_tail = NULL;
                }
                
                free(temp);
                client_found = true;
                printf("Scheduler: Dequeued client %d for processing\n", client_id);
            }
#ifdef _WIN32
            MUTEX_UNLOCK(queue_mutex);
#else
            pthread_mutex_unlock(&queue_mutex);
#endif
            
            if (!client_found) {
                // No clients waiting, sleep briefly and try again
                usleep(50000); // 50ms
                continue;
            }
        } else {
            // Round-Robin scheduling
#ifdef _WIN32
            MUTEX_LOCK(stats_mutex);
#else
            pthread_mutex_lock(&stats_mutex);
#endif
            
            // Find the next active client in a round-robin fashion
            int start_id = current_rr_client;
            
            // Only consider clients in IDLE state and active
            do {
                current_rr_client = (current_rr_client + 1) % client_count;
                
                if (client_count > 0 && 
                    client_stats[current_rr_client].state == STATE_IDLE && 
                    client_stats[current_rr_client].active) {
                    client_id = current_rr_client;
                    client_found = true;
                    break;
                }
            } while (current_rr_client != start_id && client_count > 0);
            
            if (client_found) {
                // Mark as no longer IDLE to prevent repeated attempts
                client_stats[client_id].state = STATE_CONNECTION;
                printf("Scheduler: Selected client %d (RR mode)\n", client_id);
            }
            
#ifdef _WIN32
            MUTEX_UNLOCK(stats_mutex);
#else
            pthread_mutex_unlock(&stats_mutex);
#endif
            
            if (!client_found) {
                // If no idle client is found, wait for new clients
                usleep(50000); // 50ms
                continue;
            }
        }
        
        // Get the protocol from client stats
        char protocol[10];
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        
        // Double-check client is still valid
        if (client_id >= client_count || !client_stats[client_id].active) {
#ifdef _WIN32
            MUTEX_UNLOCK(stats_mutex);
#else
            pthread_mutex_unlock(&stats_mutex);
#endif
            printf("Scheduler: Client %d is no longer valid or active\n", client_id);
            continue;
        }
        
        strncpy(protocol, client_stats[client_id].protocol, sizeof(protocol) - 1);
        protocol[sizeof(protocol) - 1] = '\0'; // Ensure null termination
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
        
        // Create a thread to handle the streaming for this client
        thread_t thread_id;
        int *client_arg = malloc(sizeof(int));
        if (client_arg == NULL) {
            perror("Memory allocation failed");
            continue;
        }
        *client_arg = client_id;
        
        if (strcasecmp(protocol, "TCP") == 0) {
            // For TCP clients, do not immediately start the streaming thread
            // The client will connect to the streaming socket, and then handle_tcp_connections
            // will create the streaming thread
            printf("Scheduler: Client %d (TCP) will connect to streaming socket\n", client_id);
            
            // Mark client as waiting for TCP connection
#ifdef _WIN32
            MUTEX_LOCK(stats_mutex);
#else
            pthread_mutex_lock(&stats_mutex);
#endif
            client_stats[client_id].state = STATE_CONNECTION;
#ifdef _WIN32
            MUTEX_UNLOCK(stats_mutex);
#else
            pthread_mutex_unlock(&stats_mutex);
#endif
            
            // Don't free client_arg as we're not using it immediately
            free(client_arg);
            
        } else if (strcasecmp(protocol, "UDP") == 0) {
            printf("Scheduler: Starting UDP streaming thread for client %d\n", client_id);
            THREAD_CREATE(thread_id, handle_udp_streaming, client_arg);
            THREAD_DETACH(thread_id);
        } else {
            printf("Unknown protocol for client %d: %s\n", client_id, protocol);
            free(client_arg);
            
            // Mark client as finished if protocol is unknown
#ifdef _WIN32
            MUTEX_LOCK(stats_mutex);
#else
            pthread_mutex_lock(&stats_mutex);
#endif
            client_stats[client_id].state = STATE_FINISHED;
            client_stats[client_id].active = 0;
#ifdef _WIN32
            MUTEX_UNLOCK(stats_mutex);
#else
            pthread_mutex_unlock(&stats_mutex);
#endif
        }
        
        // Small delay to prevent CPU hogging, but don't wait for stream to complete
        // Reduced delay to make scheduler more responsive to new clients
        usleep(5000); // 5ms delay (down from 10ms)
    }
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// Thread to accept TCP streaming connections
THREAD_RETURN_TYPE handle_tcp_connections(THREAD_PARAM arg) {
    (void)arg; // Silence unused parameter warning
    
    printf("TCP streaming acceptor thread started on port %d\n", server_port);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        socket_t client_socket = accept(tcp_streaming_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket == INVALID_SOCKET_VALUE) {
            print_socket_error("Accept failed for TCP streaming connection");
            sleep(1);
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("TCP streaming connection from %s\n", client_ip);
        
        // Set a reasonable timeout for receiving client_id
#ifdef _WIN32
        DWORD recv_timeout = 5000;  // 5 seconds in milliseconds
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_timeout, sizeof(recv_timeout));
#else
        struct timeval recv_tv;
        recv_tv.tv_sec = 5;  // 5 seconds timeout
        recv_tv.tv_usec = 0;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_tv, sizeof(recv_tv));
#endif
        
        // Receive client_id from the client
        char id_buffer[32] = {0};
        if (recv(client_socket, id_buffer, sizeof(id_buffer) - 1, 0) <= 0) {
            printf("Failed to receive client ID\n");
            print_socket_error("Receive error");
            CLOSE_SOCKET(client_socket);
            continue;
        }
        
        int client_id = atoi(id_buffer);
#ifdef _WIN32
        printf("Client %d connected for TCP streaming (socket: %" PRIu64 ")\n", 
               client_id, (uint64_t)client_socket);
#else
        printf("Client %d connected for TCP streaming (socket: %d)\n", 
               client_id, client_socket);
#endif
        
        // Validate client ID
#ifdef _WIN32
        MUTEX_LOCK(stats_mutex);
#else
        pthread_mutex_lock(&stats_mutex);
#endif
        bool valid_client = false;
        if (client_id >= 0 && client_id < client_count && 
            client_stats[client_id].active && 
            strcmp(client_stats[client_id].protocol, "TCP") == 0) {
            
            // Check if this client already has a streaming socket
            if (client_stats[client_id].socket_fd != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
                printf("Client %d already has an active streaming socket %" PRIu64 ", closing old connection\n",
                       client_id, (uint64_t)client_stats[client_id].socket_fd);
#else
                printf("Client %d already has an active streaming socket %d, closing old connection\n",
                       client_id, client_stats[client_id].socket_fd);
#endif
                CLOSE_SOCKET(client_stats[client_id].socket_fd);
            }
            
            // Update socket_fd
            client_stats[client_id].socket_fd = client_socket;
            valid_client = true;
#ifdef _WIN32
            printf("Updated socket_fd for client %d to %" PRIu64 "\n", 
                   client_id, (uint64_t)client_socket);
#else
            printf("Updated socket_fd for client %d to %d\n", 
                   client_id, client_socket);
#endif
        }
#ifdef _WIN32
        MUTEX_UNLOCK(stats_mutex);
#else
        pthread_mutex_unlock(&stats_mutex);
#endif
        
        if (!valid_client) {
            printf("Invalid TCP client ID: %d, closing connection\n", client_id);
            CLOSE_SOCKET(client_socket);
            continue;
        }
        
        // Start TCP streaming thread
        thread_t streaming_thread;
        int *client_arg = malloc(sizeof(int));
        if (client_arg == NULL) {
            perror("Memory allocation failed");
            CLOSE_SOCKET(client_socket);
            continue;
        }
        *client_arg = client_id;
        
        THREAD_CREATE(streaming_thread, handle_tcp_streaming, client_arg);
        THREAD_DETACH(streaming_thread);
    }
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// Signal handler for printing statistics
#ifdef _WIN32
BOOL WINAPI signal_handler(DWORD sig) {
    if (sig == SIGINT) {
        printf("\n\nServer shutting down, collecting statistics...\n");
        fflush(stdout);
        
        // Allow some time for any ongoing operations to complete
        Sleep(1000);
        
        // Print final statistics
        print_stats();
        
        printf("\nServer terminated gracefully.\n");
        fflush(stdout);
        
        exit(0);
        return TRUE;
    } else if (sig == SIGPIPE) {
        // Ignore SIGPIPE signals which happen when writing to a closed socket
        printf("Caught SIGPIPE (client likely disconnected) - ignoring\n");
        return TRUE;
    }
    return FALSE;
}
#else
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
    } else if (sig == SIGPIPE) {
        // Ignore SIGPIPE signals which happen when writing to a closed socket
        printf("Caught SIGPIPE (client likely disconnected) - ignoring\n");
    }
}
#endif

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 3) {
        printf("Usage: %s <Server Port> <Scheduling Policy: FCFS/RR>\n", argv[0]);
        return 1;
    }
    
    // Initialize socket system
    if (!initialize_socket_system()) {
        printf("Failed to initialize socket system\n");
        return 1;
    }
    
#ifdef _WIN32
    // Initialize mutexes and condition variables
    MUTEX_INIT(stats_mutex);
    MUTEX_INIT(queue_mutex);
    MUTEX_INIT(udp_mutex);
    MUTEX_INIT(log_mutex);
    WIN_COND_INIT(queue_cond_var);
#endif
    
    // Parse port number
    server_port = atoi(argv[1]);
    if (server_port <= 0 || server_port > 65535) {
        printf("Invalid port number. Use a number between 1 and 65535.\n");
        return 1;
    }
    
    // Parse scheduling policy
    if (strcmp(argv[2], "FCFS") == 0) {
        scheduling_policy = POLICY_FCFS;
    } else if (strcmp(argv[2], "RR") == 0) {
        scheduling_policy = POLICY_RR;
    } else {
        printf("Invalid scheduling policy. Use 'FCFS' or 'RR'.\n");
        return 1;
    }
    
    // Set up signal handlers
#ifdef _WIN32
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)signal_handler, TRUE)) {
        printf("ERROR: Could not set control handler\n");
        return 1;
    }
#else
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, signal_handler);  // Handle SIGPIPE to prevent server crash when clients disconnect
#endif

    // Initialize random seed for port generation
    srand((unsigned int)time(NULL));
    
    // Initialize client statistics
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_stats[i].active = 0;
        client_stats[i].state = STATE_IDLE;
        client_stats[i].socket_fd = INVALID_SOCKET_VALUE;
    }
    
    // Set up main TCP socket for connection phase
    socket_t server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    // Create TCP socket for connection phase
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET_VALUE) {
        print_socket_error("TCP socket creation failed");
        cleanup_socket_system();
        return 1;
    }
    
    // Set socket options to allow port reuse
#ifdef _WIN32
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
#else
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
        print_socket_error("TCP setsockopt failed");
        CLOSE_SOCKET(server_fd);
        cleanup_socket_system();
        return 1;
    }
    
    // Prepare address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(server_port);
    
    // Bind to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        print_socket_error("TCP bind failed");
        CLOSE_SOCKET(server_fd);
        cleanup_socket_system();
        return 1;
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        print_socket_error("TCP listen failed");
        CLOSE_SOCKET(server_fd);
        cleanup_socket_system();
        return 1;
    }
    
    // Create TCP socket for streaming on server_port+1
    if ((tcp_streaming_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET_VALUE) {
        print_socket_error("TCP streaming socket creation failed");
        CLOSE_SOCKET(server_fd);
        cleanup_socket_system();
        return 1;
    }
    
    // Set socket options to allow port reuse
#ifdef _WIN32
    if (setsockopt(tcp_streaming_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
#else
    if (setsockopt(tcp_streaming_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
        print_socket_error("TCP streaming setsockopt failed");
        CLOSE_SOCKET(server_fd);
        CLOSE_SOCKET(tcp_streaming_socket);
        cleanup_socket_system();
        return 1;
    }
    
    // Prepare address structure for TCP streaming socket - use server_port+1
    struct sockaddr_in tcp_streaming_addr;
    memset(&tcp_streaming_addr, 0, sizeof(tcp_streaming_addr));
    tcp_streaming_addr.sin_family = AF_INET;
    tcp_streaming_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_streaming_addr.sin_port = htons(server_port + 1); // Use server_port+1 for TCP streaming
    
    // Bind TCP streaming socket
    if (bind(tcp_streaming_socket, (struct sockaddr *)&tcp_streaming_addr, sizeof(tcp_streaming_addr)) < 0) {
        print_socket_error("TCP streaming bind failed");
        CLOSE_SOCKET(server_fd);
        CLOSE_SOCKET(tcp_streaming_socket);
        cleanup_socket_system();
        return 1;
    }
    
    // Listen for TCP streaming connections
    if (listen(tcp_streaming_socket, 10) < 0) {
        print_socket_error("TCP streaming listen failed");
        CLOSE_SOCKET(server_fd);
        CLOSE_SOCKET(tcp_streaming_socket);
        cleanup_socket_system();
        return 1;
    }
    
    // Create shared UDP socket for streaming
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET_VALUE) {
        print_socket_error("UDP socket creation failed");
        CLOSE_SOCKET(server_fd);
        CLOSE_SOCKET(tcp_streaming_socket);
        cleanup_socket_system();
        return 1;
    }
    
    // Set UDP socket options
#ifdef _WIN32
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
#else
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
        print_socket_error("UDP setsockopt failed");
        CLOSE_SOCKET(server_fd);
        CLOSE_SOCKET(tcp_streaming_socket);
        CLOSE_SOCKET(udp_socket);
        cleanup_socket_system();
        return 1;
    }
    
    // Bind UDP socket to the same port as connection phase
    struct sockaddr_in udp_address;
    memset(&udp_address, 0, sizeof(udp_address));
    udp_address.sin_family = AF_INET;
    udp_address.sin_addr.s_addr = INADDR_ANY;
    udp_address.sin_port = htons(server_port);
    
    if (bind(udp_socket, (struct sockaddr *)&udp_address, sizeof(udp_address)) < 0) {
        print_socket_error("UDP bind failed");
        CLOSE_SOCKET(server_fd);
        CLOSE_SOCKET(tcp_streaming_socket);
        CLOSE_SOCKET(udp_socket);
        cleanup_socket_system();
        return 1;
    }
    
    printf("TCP and UDP server started on port %d with %s scheduling policy\n", 
           server_port, (scheduling_policy == POLICY_FCFS) ? "FCFS" : "Round-Robin");
    printf("TCP streaming socket listening on port %d\n", server_port + 1);
    
    // Start the scheduler thread
    thread_t scheduler_id;
    if (THREAD_CREATE(scheduler_id, scheduler_thread, NULL) == 0) {
        print_socket_error("Failed to create scheduler thread");
        CLOSE_SOCKET(server_fd);
        CLOSE_SOCKET(tcp_streaming_socket);
        CLOSE_SOCKET(udp_socket);
        cleanup_socket_system();
        return 1;
    }
    
    // Start the TCP connections handler thread
    thread_t tcp_conn_id;
    if (THREAD_CREATE(tcp_conn_id, handle_tcp_connections, NULL) == 0) {
        print_socket_error("Failed to create TCP connection handler thread");
        CLOSE_SOCKET(server_fd);
        CLOSE_SOCKET(tcp_streaming_socket);
        CLOSE_SOCKET(udp_socket);
        // We should also join/terminate the scheduler thread here
        THREAD_JOIN(scheduler_id);
        cleanup_socket_system();
        return 1;
    }
    THREAD_DETACH(tcp_conn_id);
    
    // Main loop
    while (1) {
        // Accept connection from client
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        socket_t *client_socket = malloc(sizeof(socket_t));
        if (client_socket == NULL) {
            print_socket_error("Failed to allocate memory for client socket");
            continue;  // Try again
        }
        
        *client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        
        if (*client_socket == INVALID_SOCKET_VALUE) {
            print_socket_error("Accept failed");
            free(client_socket);
            continue;
        }
        
        // This is a normal connection
        thread_t thread;
        if (THREAD_CREATE(thread, handle_connection_phase, client_socket) == 0) {
            print_socket_error("Failed to create client handler thread");
            CLOSE_SOCKET(*client_socket);
            free(client_socket);
            continue;
        }
        THREAD_DETACH(thread);
    }
    
    // This point should never be reached
    CLOSE_SOCKET(server_fd);
    CLOSE_SOCKET(tcp_streaming_socket);
    CLOSE_SOCKET(udp_socket);
    
    // Clean up socket system
    cleanup_socket_system();
    
#ifdef _WIN32
    // Destroy mutexes and condition variables
    MUTEX_DESTROY(stats_mutex);
    MUTEX_DESTROY(queue_mutex);
    MUTEX_DESTROY(udp_mutex);
    MUTEX_DESTROY(log_mutex);
    WIN_COND_DESTROY(queue_cond_var);
#endif
    
    return 0;
} 