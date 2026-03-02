#include "client.h"

#include <server/models.h>
#include <util.h>

#include "../world/core/chunk.h"
#include "compression/compression.h"

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


#pragma region thread_management

static pthread_key_t  thread_socket_key;
static pthread_once_t thread_socket_key_once = PTHREAD_ONCE_INIT;

static void close_thread_socket(void* ptr) {
    int* fd_ptr = (int*)ptr;
    if (fd_ptr) {
        if (*fd_ptr >= 0) close(*fd_ptr);
        free(fd_ptr);
    }
}

static void init_thread_socket_key(void) {
    pthread_key_create(&thread_socket_key, close_thread_socket);
}

// Returns this thread's socket fd, connecting (or reconnecting) as needed.
static int get_thread_socket(void) {
    pthread_once(&thread_socket_key_once, init_thread_socket_key);

    int* fd_ptr = (int*)pthread_getspecific(thread_socket_key);
    if (fd_ptr == NULL) {
        fd_ptr = malloc(sizeof(int));
        *fd_ptr = -1;
        pthread_setspecific(thread_socket_key, fd_ptr);
    }

    // Return the cached fd if it's valid.
    if (*fd_ptr >= 0) {
        return *fd_ptr;
    }

    // Connect (or reconnect after a previous failure or broken socket).
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Failed to create client socket\n");
        return -1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(SERVER_PORT)
    };
    inet_pton(AF_INET, SERVER_HOST, &addr.sin_addr);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "ERROR: Failed to connect to chunk server\n");
        close(fd);
        return -1;
    }

    *fd_ptr = fd;
    return fd;
}

// Mark this thread's socket as broken so the next get_thread_socket() reconnects.
static void reset_thread_socket(void) {
    int* fd_ptr = (int*)pthread_getspecific(thread_socket_key);
    if (fd_ptr && *fd_ptr >= 0) {
        close(*fd_ptr);
        *fd_ptr = -1;
    }
}

#pragma endregion

bool send_chunk_request(int fd, int x, int z) {

    chunk_msg_type msg_type = CHUNK_REQ;
    if (send(fd, &msg_type, sizeof(chunk_msg_type), MSG_NOSIGNAL) < 0) {
        printf("ERROR: Failed to send request metadata to server\n.");
    }
    
    chunk_request req = {
        .x = x,
        .z = z
    };
    if (send(fd, &req, sizeof(req), MSG_NOSIGNAL) < 0) {
        fprintf(stderr, "ERROR: Failed to send chunk request to server\n");
        return false;
    }

    return true;
}

chunk* request_chunk(int x, int z) {
    int fd = get_thread_socket();
    send_chunk_request(fd, x, z);

    int packet_size = -1;
    if (recv(fd, &packet_size, sizeof(int), MSG_WAITALL) != sizeof(int) || packet_size <= 0) {
        printf("ERROR: Failed to receive chunk size from server\n");
        return NULL;
    }

    byte* compressed_chunk = malloc(packet_size);
    if (compressed_chunk == NULL) {
        printf("ERROR: Could not allocate buffer for compressed chunk\n");
        return NULL;
    }
    if (recv(fd, compressed_chunk, packet_size, MSG_WAITALL) != packet_size) {
        printf("ERROR: Could not receive compressed chunk from server (fd %i)\n", fd);
        free(compressed_chunk);
        return NULL;
    }

    chunk* c = decompress_chunk(compressed_chunk, packet_size);
    free(compressed_chunk);
    return c;
}