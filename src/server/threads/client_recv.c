#include "client_recv.h"

#include <stdio.h>
#include <server/models.h>
#include <server/client.h>
#include <util.h>

#include "../../world/core/world.h"
#include "../compression/compression.h"

#include <sys/socket.h>
#include <stdlib.h>

static void process_chunk_broadcast(int fd) {
    int packet_size = -1;
    if (recv(fd, &packet_size, sizeof(int), MSG_WAITALL) <= 0) {
        printf("ERROR: Failed to receive broadcast size from server\n");
        return;
    }

    if (packet_size <= 0) {
        printf("ERROR: Invalid broadcast packet size (%d)\n", packet_size);
        return;
    }

    byte* data = malloc(packet_size);
    if (data == NULL || recv(fd, data, packet_size, MSG_WAITALL) != packet_size) {
        printf("ERROR: Failed to receive broadcast chunk data from server\n");
        free(data);
        return;
    }

    chunk* c = decompress_chunk(data, packet_size);
    free(data);

    if (c == NULL) {
        printf("ERROR: Failed to decompress broadcast chunk\n");
        return;
    }

    update_chunk(c);
}

void* run_client_recv_thread(void* args) {
    (void)args;

    int fd = acquire_server_fd();
    if (fd < 0) {
        printf("ERROR: Client recv thread could not connect to server\n");
        return NULL;
    }

    while (1) {
        chunk_msg_type msg_type = UNKNOWN;
        if (recv(fd, &msg_type, sizeof(chunk_msg_type), MSG_WAITALL) <= 0) {
            printf("Server disconnected (client recv thread)\n");
            break;
        }

        switch (msg_type) {
            case CHUNK_BROADCAST:
                process_chunk_broadcast(fd);
                break;
            default:
                printf("ERROR: Unexpected message type in client recv thread (%d)\n", msg_type);
                break;
        }
    }

    return NULL;
}
