
#include "server.h"

#include "../world/core/chunk.h"
#include "../world/core/chunk_io.h"
#include "../world/core/world.h"
#include "../util/settings.h"
#include "compression/compression.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>


int listen_fd = 0;
struct pollfd fds[MAX_CLIENTS + 1];
int num_fds = 0;

void check_new_connections(void) {
    if (!(fds[0].revents & POLLIN)) {
        return;
    }

    int client_fd = accept(listen_fd, NULL, NULL);

    if (num_fds < MAX_CLIENTS + 1) {
        fds[num_fds].fd = client_fd;
        fds[num_fds].events = POLLIN;
        num_fds++;
        printf("Client connected (fd %d)\n", client_fd);
    }
    else {
        printf("Closing connection: server full (fd %d)\n", client_fd);
        close(client_fd);
    }
}

void start_server(void) {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;

    num_fds++;

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_fd, 1);

    printf("Waiting for connection on port 8080...\n");

    int running = 1;

    while (running) {
        int ready = poll(fds, num_fds, 16);

        if (ready < 0) {
            printf("ERROR: Error polling connections...\n");
            return;
        }

        check_new_connections();

        for (int i = 1; i < num_fds; i++) {
            if (!(fds[i].revents & POLLIN)) {
                continue;
            }

            chunk_request* req = (chunk_request*)malloc(sizeof(chunk_request));

            int n = recv(fds[i].fd, req, sizeof(chunk_request), 0);

            if (n <= 0) {
                printf("Client disconnected (fd %s)\n", fds[i].fd);
                close(fds[i].fd);
                fds[i] = fds[num_fds - 1];
                num_fds--;
                i--;
            } else {
                printf("Server received request for chunk (%d, %d)\n", req->x, req->z);
                
                chunk* c = malloc(sizeof(chunk));

                if (chunk_load_from_disk(c, WORLDS_DIR) == -1) {
                    chunk_create(c, req->x, req->z);
                }

                int packet_size = 0;
                byte* c_comp = compress_chunk(c, &packet_size);
                send(fds[i].fd, &packet_size, sizeof(int), 0);
                send(fds[i].fd, &c_comp, packet_size, 0);

                // TODO: cache chunk

                free(c);
            }
        }
    }
}