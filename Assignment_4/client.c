#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <inttypes.h>

// Platform-specific includes and definitions
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    // #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define strcasecmp _stricmp
    #define sleep(x) Sleep(x*1000)
    #define usleep(x) Sleep(x/1000)
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define CLOSE_SOCKET(s) closesocket(s)
    #define GET_LAST_ERROR() WSAGetLastError()
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/time.h>
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE -1
    #define CLOSE_SOCKET(s) close(s)
    #define GET_LAST_ERROR() errno
#endif

// Add socket format printing helpers for Windows - function declarations
#ifdef _WIN32
void print_socket_info(const char* prefix, socket_t socket_val);
#else
void print_socket_info(const char* prefix, socket_t socket_val);
#endif

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
void print_socket_info(const char* prefix, socket_t socket_val) {
    printf("%s%" PRIu64 "\n", prefix, (uint64_t)socket_val);
}
#else
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

// Send Type 1 Request and receive Type 2 Response using TCP
int connection_phase(const char *server_ip, int server_port, const char *resolution, const char *protocol, int *streaming_port) {
    socket_t sock = INVALID_SOCKET_VALUE;
    struct sockaddr_in serv_addr;
    Message request, response;
    
    // Create TCP socket for connection phase
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET_VALUE) {
        print_socket_error("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        print_socket_error("Invalid address or address not supported");
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        print_socket_error("TCP connection failed");
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server at %s:%d for connection phase\n", server_ip, server_port);
    
    // Prepare Type 1 Request
    request.type = TYPE_1_REQUEST;
    strncpy(request.resolution, resolution, sizeof(request.resolution));
    strncpy(request.protocol, protocol, sizeof(request.protocol));
    request.bandwidth = 0; // Client doesn't set bandwidth
    
    // Send Type 1 Request
    if (send(sock, (char*)&request, sizeof(request), 0) < 0) {
        print_socket_error("Failed to send request message");
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Sent Type 1 Request with resolution: %s and protocol: %s\n", resolution, protocol);
    
    // Receive Type 2 Response
    int bytes_received = recv(sock, (char*)&response, sizeof(response), 0);
    if (bytes_received <= 0) {
        print_socket_error("Failed to receive response from server");
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    if (response.type != TYPE_2_RESPONSE) {
        printf("Received invalid response type\n");
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    // Extract streaming port and client_id assigned by server
    *streaming_port = response.streaming_port;
    
    printf("Received Type 2 Response - Selected resolution: %s, Bandwidth requirement: %d Kbps, Streaming port: %d, Client ID: %d\n", 
           response.resolution, response.bandwidth, *streaming_port, response.client_id);
    
    // Close the connection phase socket
    CLOSE_SOCKET(sock);
    
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
    
    socket_t sock = INVALID_SOCKET_VALUE;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Create socket for video streaming
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET_VALUE) {
        print_socket_error("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(streaming_port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        print_socket_error("Invalid address or address not supported");
        CLOSE_SOCKET(sock);
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
            print_socket_error("Connection attempt failed");
            printf("Connection attempt %d failed, retrying in 1 second...\n", retry_count);
            sleep(1);
        }
    }
    
    if (!connected) {
        print_socket_error("TCP connection failed after multiple attempts");
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to TCP server at %s:%d for video streaming\n", server_ip, streaming_port);
    
    // Print socket info using our platform-safe function
    print_socket_info("Using socket: ", sock);
    
    // Send client_id that was assigned by the server
    char id_buffer[32];
    snprintf(id_buffer, sizeof(id_buffer), "%d", client_id);
    
    printf("Sending client ID: %s\n", id_buffer);
    if (send(sock, id_buffer, strlen(id_buffer), 0) < 0) {
        print_socket_error("Failed to send client ID");
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    // Wait for server to be ready with timeout
#ifdef _WIN32
    DWORD timeout = 15000;  // 15 seconds in milliseconds
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 15;  // Increased timeout to 15 seconds
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif
    
    // Wait for server to be ready
    printf("Waiting for server READY_TO_STREAM message (timeout: 15 seconds)...\n");
    int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    
    if (bytes_received <= 0) {
        printf("Server not ready to stream (timeout after 15 seconds)\n");
        print_socket_error("Error");
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Received message from server: '%s'\n", buffer);
    
    if (strcmp(buffer, "READY_TO_STREAM") != 0) {
        printf("Server sent unexpected message: '%s' instead of 'READY_TO_STREAM'\n", buffer);
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Received READY_TO_STREAM from server\n");
    
    // Send confirmation to start streaming
    printf("Sending START_STREAM confirmation to server\n");
    if (send(sock, "START_STREAM", strlen("START_STREAM"), 0) < 0) {
        print_socket_error("Failed to send START_STREAM confirmation");
        CLOSE_SOCKET(sock);
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
    CLOSE_SOCKET(sock);
}

// UDP client implementation for video streaming
void udp_client(const char *server_ip, int server_port, const char *resolution) {
    int streaming_port = server_port; // Default value
    int client_id = connection_phase(server_ip, server_port, resolution, "UDP", &streaming_port);
    int bandwidth = 6000; // Default value, will be set by the server
    
    socket_t sock = INVALID_SOCKET_VALUE;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET_VALUE) {
        print_socket_error("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(streaming_port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        print_socket_error("Invalid address or address not supported");
        CLOSE_SOCKET(sock);
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
            print_socket_error("Failed to send UDP request");
            retry_count++;
            sleep(1);
            continue;
        }
        
        printf("Sent REQUEST_STREAM to server (attempt %d/%d)\n", retry_count + 1, max_retries);
        
        // Wait for server to be ready with a timeout
#ifdef _WIN32
        DWORD timeout = 3000;  // 3 seconds in milliseconds
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        struct timeval tv;
        tv.tv_sec = 3;  // 3 seconds timeout for initial response
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif
        
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
            print_socket_error("No response from server");
            printf("Retry %d/%d: No response from server\n", retry_count + 1, max_retries);
            retry_count++;
        }
    }
    
    if (!received_ready) {
        printf("Server not ready to stream after %d attempts\n", max_retries);
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Starting video stream reception (UDP, %s)...\n", resolution);
    
    // Set longer timeout for video streaming
#ifdef _WIN32
    DWORD stream_timeout = 5000;  // 5 seconds in milliseconds
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&stream_timeout, sizeof(stream_timeout));
#else
    struct timeval stream_tv;
    stream_tv.tv_sec = 5;  // 5 seconds timeout
    stream_tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&stream_tv, sizeof(stream_tv));
#endif
    
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
        
#ifdef _WIN32
        DWORD timeout = 5000;  // 5 seconds in milliseconds
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#endif

        socklen_t server_addr_len = sizeof(serv_addr);
        
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
    CLOSE_SOCKET(sock);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <Server IP> <Server Port> <Resolution: 480p/720p/1080p> <Mode: TCP/UDP>\n", argv[0]);
        return -1;
    }
    
    // Initialize socket system (needed for Windows)
    if (!initialize_socket_system()) {
        printf("Failed to initialize socket system\n");
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
        cleanup_socket_system();
        return -1;
    }
    
    // Validate mode and call appropriate client function
    if (strcasecmp(mode, "TCP") == 0) {
        tcp_client(server_ip, server_port, resolution);
    } else if (strcasecmp(mode, "UDP") == 0) {
        udp_client(server_ip, server_port, resolution);
    } else {
        printf("Invalid mode. Use 'TCP' or 'UDP'.\n");
        cleanup_socket_system();
        return -1;
    }
    
    // Clean up socket system
    cleanup_socket_system();
    
    return 0;
} 