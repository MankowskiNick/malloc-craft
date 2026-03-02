#include "listen.h"

#include <stdio.h>
#include <server/models.h>
#include <util.h>

#include "../../world/core/chunk.h"
#include "../../world/core/world.h"
#include "../../util/settings.h"
#include "../compression/compression.h"
#include "../world/world_state.h"
#include "../world/chunk_io.h"
#include "broadcast.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

void handle_disconnect(client_connection* c) {
    printf("Client disconnected(fd %d)\n", c->fd);
    
    c->fd = -1;
}

void process_chunk_request(client_connection* client) {
    chunk_request req;
    if (recv(client->fd, &req, sizeof(chunk_request), MSG_WAITALL) <= 0) {
        handle_disconnect(client);
        return;
    } 

    chunk* c = load_chunk_state(&(client->parent->disk_lock), req.x, req.z);

    int packet_size = 0;
    byte* compressed_chunk = compress_chunk(c, &packet_size);

    if (!send_data(client->fd, &packet_size, sizeof(int)) ||
        !send_data(client->fd, compressed_chunk, packet_size)) {
        printf("ERROR: Failed to send chunk to client (fd %d)\n", client->fd);
    }
    free(compressed_chunk);
    free(c);
}

void process_chunk_update(client_connection* client) {
    int packet_size = -1;
    if (recv(client->fd, &packet_size, sizeof(int), MSG_WAITALL) <= 0) {
        handle_disconnect(client);
        return;
    }

    if (packet_size < 0) {
        printf("ERROR: Packet size cannot be negative(packet_size = %i)\n", packet_size);
        handle_disconnect(client);
        return;
    }

    byte* compressed_chunk = malloc(packet_size);
    if (compressed_chunk == NULL || recv(client->fd, compressed_chunk, packet_size, MSG_WAITALL) <= 0) {
        printf("ERROR: Failed to allocate/receive chunk from client(fd = %i).\n", client->fd);
        handle_disconnect(client);
        return;
    }

    add_to_broadcast_queue(compressed_chunk, packet_size);
    chunk* c = decompress_chunk(compressed_chunk, packet_size);

    if (c == NULL) {
        printf("ERROR: Failed to decompress chunk\n");
        return;
    }

    save_chunk_state(c);
    
    free(compressed_chunk);
    free(c);
}

void* run_listen_thread(void* args) {
    client_connection* client = (client_connection*)args;

    // Initialize the worlds directory
    if (init_worlds_directory(WORLDS_DIR) == -1) {
        fprintf(stderr, "Warning: Failed to initialize worlds directory\n");
    }

    while (1) {
        chunk_msg_type msg_type = UNKNOWN;
        if (recv(client->fd, &msg_type, sizeof(chunk_msg_type), MSG_WAITALL) <= 0) {
            handle_disconnect(client);
            break;
        }

        switch(msg_type) {
            case CHUNK_REQ:
                process_chunk_request(client);
                break;
            case CHUNK_UPDATE:
                process_chunk_update(client);
                break;
            default:
                printf("ERROR: Unknown message header incoming from client %i(header=%i)", client->fd, msg_type);
                break;
        }
    }
}