
#include <server/server.h>

#include "../util/settings.h"
#include "threads/listen.h"
#include "threads/broadcast.h"

#include <server/models.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


int configure_fd(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("ERROR: Failed to create server socket\n");
        return fd;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(fd, MAX_CLIENTS);
    return fd;
}

client_connection* create_client(int fd, server_t* server) {
    if (server->client_count >= MAX_CLIENTS) {
        printf("ERROR: Max clients reached\n");
        close(fd);
        return NULL;
    }

    client_connection* client = &server->clients[server->client_count];
    client->fd = fd;
    client->parent = server;

    if (pthread_create(&client->recv_thread, NULL, run_listen_thread, client) != 0) {
        printf("ERROR: Failed to create client listener thread\n");
        close(fd);
        client->fd = -1;
        return NULL;
    }

    server->client_count++;
    return client;
}

server_t* create_server() {

    server_t* server = malloc(sizeof(server_t));
    server->listen_fd = configure_fd();
    server->client_count = 0;
    pthread_mutex_init(&server->disk_lock, NULL);

    if (pthread_create(&server->broadcast_thread, NULL, run_broadcast_thread, server) != 0) {
        printf("ERROR: Failed to create broadcast thread.\n");
        close(server->listen_fd);
        return NULL;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        server->clients[i].fd = -1;
        server->clients[i].parent = server;
    }

    return server;
}

void start_server(void) {

    server_t* server = create_server();

    printf("Waiting for connections on port %d...\n", SERVER_PORT);

    while (1) {
        int client_fd = accept(server->listen_fd, NULL, NULL);
        if (client_fd < 0) {
            printf("ERROR: Failed to accept connection\n");
            continue;
        }

        client_connection* client = create_client(client_fd, server);
        if (client == NULL) {
            continue;
        }
        printf("Client connected (fd %d)\n", client->fd);
    }

    free(server); // more to free for sure
}
