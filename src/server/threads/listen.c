#include "listen.h"

#include <stdio.h>
#include <server/models.h>
#include <util.h>

#include "../../world/core/chunk.h"
#include "../../world/core/chunk_io.h"
#include "../../world/core/world.h"
#include "../../util/settings.h"
#include "../compression/compression.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

static bool send_all(int fd, const void* buf, int len) {
    const byte* ptr = (const byte*)buf;
    while (len > 0) {
        int sent = send(fd, ptr, len, MSG_NOSIGNAL);
        if (sent <= 0) return false;
        ptr += sent;
        len -= sent;
    }
    return true;
}

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
        
    chunk* c = malloc(sizeof(chunk));
    c->x = req.x;
    c->z = req.z;

    pthread_mutex_lock(&(client->parent->disk_lock));
    int chunk_read = chunk_load_from_disk(c, WORLDS_DIR);
    pthread_mutex_unlock(&(client->parent->disk_lock));

    if (chunk_read == -1) {
        chunk_create(c, req.x, req.z);
    }


    int packet_size = 0;
    byte* c_comp = compress_chunk(c, &packet_size);

    if (!send_all(client->fd, &packet_size, sizeof(int)) ||
        !send_all(client->fd, c_comp, packet_size)) {
        printf("ERROR: Failed to send chunk to client (fd %d)\n", client->fd);
    }
    free(c_comp);
    free(c);
}

void process_chunk_update(void) {

}

void* run_listen_thread(void* args) {
    client_connection* client = (client_connection*)args;

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
                process_chunk_update();
                break;
            default:
                printf("ERROR: Unknown message header incoming from client %i(header=%i)", client->fd, msg_type);
                break;
        }
    }
}