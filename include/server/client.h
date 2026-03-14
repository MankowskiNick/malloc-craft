#ifndef CLIENT_H
#define CLIENT_H

#include <block_models.h>
#include <util.h>

#include "../world/core/chunk.h"

int acquire_server_fd(void);
chunk* request_chunk(int x, int z);
void send_chunk_to_server(chunk* c);

#endif