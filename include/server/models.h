#ifndef SERVER_MODELS_H
#define SERVER_MODELS_H


#include <util.h>
#include <pthread.h>

#define MAX_CLIENTS 32

typedef enum chunk_msg_type {
    UNKNOWN,
    CHUNK_REQ,
    CHUNK_UPDATE,
    CHUNK_BROADCAST
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

#endif