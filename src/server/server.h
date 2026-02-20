#ifndef SERVER_H
#define SERVER_H

#define PORT 8085 // TODO: move this to settings.json
#define MAX_CLIENTS 32

typedef struct chunk_request {
    int x, z;
} chunk_request;

void start_server(void);

#endif
