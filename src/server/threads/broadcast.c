#include "broadcast.h"

#include <server/models.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

pthread_mutex_t broadcast_queue_lock = PTHREAD_MUTEX_INITIALIZER;
struct broadcast_queue_node* queue_head = NULL;
struct broadcast_queue_node* queue_tail = NULL;

struct broadcast_queue_node {
    struct broadcast_queue_node* next;
    byte* data;
    int size;
};

bool send_to_client(int fd, byte* data, int size) {
    if (!send_msg_header(CHUNK_BROADCAST, fd) ||
        !send_data(fd, &size, sizeof(int)) ||
        !send_data(fd, data, size)) {
        printf("ERROR: Failed to send chunk to client (fd %d)\n", fd);
        return false;
    }
    return true;
}

void broadcast_data(server_t* server, byte* data, int size) {
    if (data == NULL || size < 0) {
        printf("ERROR: Invalid data to broadcast.\n");
        return;
    }

    for (int i = 0; i < server->client_count; i++) {
        if (!server->clients[i].is_broadcast_subscriber) continue;
        send_to_client(server->clients[i].fd, data, size);
    }
}

void add_to_broadcast_queue(byte* data, int size) {
    byte* copied_data = malloc(size);
    memcpy(copied_data, data, size);

    struct broadcast_queue_node* node = malloc(sizeof(struct broadcast_queue_node));
    node->data = copied_data;
    node->size = size;
    node->next = NULL;

    pthread_mutex_lock(&broadcast_queue_lock);
    if (queue_tail == NULL) {
        queue_head = node;
        queue_tail = node;
    } else {
        queue_tail->next = node;
        queue_tail = node;
    }
    pthread_mutex_unlock(&broadcast_queue_lock);
}

void* run_broadcast_thread(void* args) {
    server_t* server = (server_t*)args;

    if (server == NULL) {
        printf("ERROR: No server parameter passed to broadcast thread.\n");
        return NULL;
    }

    while (1) {
        pthread_mutex_lock(&broadcast_queue_lock);
        struct broadcast_queue_node* cur = queue_head;
        if (cur != NULL) {
            queue_head = cur->next;
            if (queue_head == NULL) {
                queue_tail = NULL;
            }
        }
        pthread_mutex_unlock(&broadcast_queue_lock);

        if (cur == NULL) {
            usleep(100); // TODO: this should be configurable
            continue;
        }

        broadcast_data(server, cur->data, cur->size);

        free(cur->data);
        free(cur);
    }
}