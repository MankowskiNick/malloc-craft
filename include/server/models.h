#ifndef SERVER_MODELS_H
#define SERVER_MODELS_H


#include <util.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_CLIENTS 640

typedef enum chunk_msg_type {
    UNKNOWN,
    CHUNK_REQ,
    CHUNK_UPDATE,
    CHUNK_BROADCAST,
    SUBSCRIBE_BROADCAST
} chunk_msg_type;

typedef struct chunk_request {
    int x, z;
} chunk_request;

typedef struct chunk_packet {
    chunk_msg_type type;
    byte* data;
    size_t size;
} chunk_packet;

struct server_t;
typedef struct client_connection {
    int fd;
    bool is_broadcast_subscriber;
    pthread_t recv_thread;
    struct server_t* parent;
} client_connection;

typedef struct server_t {
    int listen_fd;

    pthread_t broadcast_thread;
    pthread_mutex_t disk_lock;

    client_connection clients[MAX_CLIENTS];
    size_t client_count;
} server_t;

static bool send_data(int fd, const void* buf, int len) {
    const byte* ptr = (const byte*)buf;
    while (len > 0) {
        int sent = send(fd, ptr, len, MSG_NOSIGNAL);
        if (sent <= 0) return false;
        ptr += sent;
        len -= sent;
    }
    return true;
}

static bool send_msg_header(chunk_msg_type type, int fd) {
    if (send(fd, &type, sizeof(chunk_msg_type), MSG_NOSIGNAL) < 0) {
        printf("ERROR: Failed to send request metadata to server\n.");
        return false;
    }
    return true;
}

#endif