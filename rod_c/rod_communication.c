/**
 * @file rod_communication.c
 * @brief Communication thread for receiving detection data and transmitting to robot
 * @author Noé Game
 * @date 15/02/2026
 * @see rod_communication.c
 * @copyright Cecill-C (Cf. LICENCE.txt)
 * 
 * This program implements the communication thread that:
 * - Connects to the detection thread via Unix socket
 * - Receives detected marker positions [[id, x, y, angle], ...]
 * - Prints detection data to console
 * - Will eventually transmit data to the robot's main process
 */

/* ******************************************************* Includes ****************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <poll.h>

/* ***************************************************** Public macros *************************************************** */

// Socket configuration (must match rod_detection.c)
#define SOCKET_PATH "/tmp/rod_detection.sock"
#define MAX_BUFFER_SIZE 4096
#define RECONNECT_DELAY_US 1000000  // 1 second in microseconds
#define POLL_TIMEOUT_MS 100         // 100ms poll timeout for responsive shutdown

/* ************************************************** Public types definition ******************************************** */

/**
 * @brief Application context for communication thread
 */
typedef struct {
    int socket_fd;
    bool running;
    bool connected;
} CommContext;

/* *********************************************** Public functions declarations ***************************************** */

/**
 * @brief Initialize communication context
 * @param ctx Communication context
 */
static void init_comm_context(CommContext* ctx);

/**
 * @brief Connect to the detection thread socket
 * @param ctx Communication context
 * @return 0 on success, -1 on failure
 */
static int connect_to_detection_socket(CommContext* ctx);

/**
 * @brief Cleanup communication context
 * @param ctx Communication context
 */
static void cleanup_comm_context(CommContext* ctx);

/**
 * @brief Process received detection data
 * @param data Received data string
 */
static void process_detection_data(const char* data);

/**
 * @brief Signal handler for graceful shutdown
 * @param signum Signal number
 */
static void signal_handler(int signum);

/* ******************************************* Global variables ******************************************************* */

static volatile bool g_running = true;

/* ********************************************* Function implementations *********************************************** */

static void signal_handler(int signum) {
    (void)signum;
    g_running = false;
    printf("\nReceived interrupt signal, shutting down...\n");
}

static void init_comm_context(CommContext* ctx) {
    ctx->socket_fd = -1;
    ctx->running = true;
    ctx->connected = false;
}

static int connect_to_detection_socket(CommContext* ctx) {
    struct sockaddr_un addr;
    
    // Close existing connection if any
    if (ctx->socket_fd >= 0) {
        close(ctx->socket_fd);
        ctx->socket_fd = -1;
        ctx->connected = false;
    }
    
    // Create socket
    ctx->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ctx->socket_fd < 0) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    
    // Setup socket address
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    // Connect to detection socket
    if (connect(ctx->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Failed to connect to %s: %s\n", SOCKET_PATH, strerror(errno));
        close(ctx->socket_fd);
        ctx->socket_fd = -1;
        return -1;
    }
    
    ctx->connected = true;
    printf("Successfully connected to detection socket: %s\n", SOCKET_PATH);
    return 0;
}

static void cleanup_comm_context(CommContext* ctx) {
    if (ctx->socket_fd >= 0) {
        close(ctx->socket_fd);
        ctx->socket_fd = -1;
    }
    ctx->connected = false;
}

static void process_detection_data(const char* data) {
    // Print received detection data
    printf("Received detection data: %s", data);
    
    // TODO: In the future, this function will:
    // - Parse the JSON-like array format [[id, x, y, angle], ...]
    // - Transform coordinates to robot coordinate system
    // - Send data to robot's main process via appropriate protocol
    // - Handle acknowledgments and retransmissions
}

/**
 * @brief Main function of the communication thread
 * Connects to the detection thread socket and receives marker detection data.
 */
int main(int argc, char* argv[]) {
    CommContext ctx;
    char buffer[MAX_BUFFER_SIZE];
    
    (void)argc;  // Unused
    (void)argv;  // Unused
    
    printf("=== ROD Communication - IPC Thread ===\n");
    printf("Waiting for detection data from %s\n\n", SOCKET_PATH);
    
    // Setup signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize context
    init_comm_context(&ctx);
    
    // Main communication loop
    while (g_running && ctx.running) {
        // Try to connect if not connected
        if (!ctx.connected) {
            printf("Attempting to connect to detection socket...\n");
            if (connect_to_detection_socket(&ctx) != 0) {
                printf("Connection failed, retrying in 1 second...\n");
                usleep(RECONNECT_DELAY_US);
                continue;
            }
        }
        
        // Use poll() to wait for data with timeout (allows responsive shutdown)
        struct pollfd pfd;
        pfd.fd = ctx.socket_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        
        int poll_result = poll(&pfd, 1, POLL_TIMEOUT_MS);
        
        if (poll_result < 0) {
            // Poll error
            if (errno == EINTR) {
                // Interrupted by signal, check g_running and continue
                continue;
            }
            fprintf(stderr, "Poll error: %s\n", strerror(errno));
            cleanup_comm_context(&ctx);
            usleep(RECONNECT_DELAY_US);
            continue;
            
        } else if (poll_result == 0) {
            // Timeout - no data available, loop back to check g_running
            continue;
            
        } else {
            // Data available or connection event
            if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
                // Connection closed or error
                printf("Detection thread closed connection, reconnecting...\n");
                cleanup_comm_context(&ctx);
                usleep(RECONNECT_DELAY_US);
                continue;
            }
            
            if (pfd.revents & POLLIN) {
                // Data available - receive it
                ssize_t bytes_received = recv(ctx.socket_fd, buffer, MAX_BUFFER_SIZE - 1, 0);
                
                if (bytes_received > 0) {
                    // Null-terminate received data
                    buffer[bytes_received] = '\0';
                    
                    // Process the detection data
                    process_detection_data(buffer);
                    
                } else if (bytes_received == 0) {
                    // Connection closed by detection thread
                    printf("Detection thread closed connection, reconnecting...\n");
                    cleanup_comm_context(&ctx);
                    usleep(RECONNECT_DELAY_US);
                    
                } else {
                    // Error occurred
                    fprintf(stderr, "Error receiving data: %s\n", strerror(errno));
                    cleanup_comm_context(&ctx);
                    usleep(RECONNECT_DELAY_US);
                }
            }
        }
    }
    
    printf("\nShutting down communication thread...\n");
    cleanup_comm_context(&ctx);
    printf("ROD Communication stopped successfully\n");
    
    return 0;
}
