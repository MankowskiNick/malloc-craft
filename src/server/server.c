
#include "server.h"

#include "../world/core/chunk.h"
#include "../world/core/chunk_io.h"
#include "../world/core/world.h"
#include "../util/settings.h"
#include "compression/compression.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

static int listen_fd = -1;

static void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);

    printf("Client connected (fd %d)\n", client_fd);

    while (1) {
        chunk_request req;
        int n = recv(client_fd, &req, sizeof(chunk_request), MSG_WAITALL);
        if (n <= 0) {
            printf("Client disconnected (fd %d)\n", client_fd);
            break;
        }

        chunk* c = malloc(sizeof(chunk));
        c->x = req.x;
        c->z = req.z;

        if (chunk_load_from_disk(c, WORLDS_DIR) == -1) {
            chunk_create(c, req.x, req.z);
            chunk_save_to_disk(c, WORLDS_DIR);
        }

        int packet_size = 0;
        byte* c_comp = compress_chunk(c, &packet_size);
        printf("Sending %i bytes to client...\n", packet_size);
        if (send(client_fd, &packet_size, sizeof(int), MSG_NOSIGNAL) < 0 ||
            send(client_fd, c_comp, packet_size, MSG_NOSIGNAL) < 0) {
            printf("ERROR: Failed to send chunk to client (fd %d)\n", client_fd);
        }

        free(c_comp);
        free(c);
    }

    close(client_fd);
    return NULL;
}

void start_server(void) {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        printf("ERROR: Failed to create server socket\n");
        return;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, MAX_CLIENTS);

    printf("Waiting for connections on port %d...\n", PORT);

    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
            printf("ERROR: Failed to accept connection\n");
            continue;
        }

        int* fd_ptr = malloc(sizeof(int));
        if (fd_ptr == NULL) {
            close(client_fd);
            continue;
        }
        *fd_ptr = client_fd;

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, fd_ptr) != 0) {
            printf("ERROR: Failed to create client thread\n");
            free(fd_ptr);
            close(client_fd);
            continue;
        }
        pthread_detach(thread);
    }
}
