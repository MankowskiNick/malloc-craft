#include "world.h"
#include "chunk.h"
#include "chunk_io.h"
#include "../../server/server.h"
#include "../../server/compression/compression.h"
#include <hashmap.h>


#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define mod(x, y) fmod(x, y) < 0 ? fmod(x, y) + (y) : fmod(x,y)

int server_fd = 0;

DEFINE_HASHMAP(chunk_map, chunk_coord, chunk*, chunk_hash, chunk_equals);
typedef chunk_map_hashmap chunk_map;
chunk_map chunks;

const char* get_worlds_dir(void) {
    return WORLDS_DIR;
}

void w_init() {
    c_init();

    chunks = chunk_map_init(CHUNK_CACHE_SIZE);
    
    // Initialize the worlds directory
    if (init_worlds_directory(WORLDS_DIR) == -1) {
        fprintf(stderr, "Warning: Failed to initialize worlds directory\n");
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(server_fd, (struct sockaddr*)&addr, sizeof(addr));
}

void w_cleanup() {
    for (size_t i = 0; i < chunks.capacity; ++i) {
        chunk_map_entry* current = chunks.buckets[i];
        while (current) {
            chunk* c = current->value;
            
            if (c->modified) {
                if (chunk_save_to_disk(c, WORLDS_DIR) == -1) {
                    fprintf(stderr, "Warning: Failed to save chunk at (%d, %d)\n", c->x, c->z);
                }
            }
            
            free(c);  // Free the chunk pointer
            current = current->next;
        }
    }
    chunk_map_free(&chunks);
    // close connection? probably.
}

chunk* get_chunk(int x, int z) {
    chunk_coord coord = {x, z};
    chunk** found = chunk_map_get(&chunks, coord);
    chunk* c = found ? *found : NULL;
    if (c == NULL) {
        c = malloc(sizeof(chunk));
        
        c->x = x;
        c->z = z;
        if (chunk_load_from_disk(c, WORLDS_DIR) == -1) {
            chunk_create(c, x, z);
        }
        
        chunk_request req = {
            .x = x,
            .z = z
        };
        send(server_fd, &req, sizeof(req), 0);

        int packet_size = 0;
        int n1 = recv(server_fd, &packet_size, sizeof(int), 0);
        byte* c_comp = malloc(packet_size);

        c = malloc(sizeof(chunk));
        int n2 = recv(server_fd, &c_comp, packet_size, 0);
        c = decompress_chunk(c_comp, packet_size);

        chunk_map_insert(&chunks, coord, c);
    }
    return c;
}

chunk* get_chunk_at(float x, float z, int* chunk_x, int* chunk_z) {
    
    // adjust for negative coordinates
    if (x < 0) {
        x -= CHUNK_SIZE;
    }
    if (z < 0) {
        z -= CHUNK_SIZE;
    }
    // Calculate chunk coordinates
    int cx = (int)(x / CHUNK_SIZE);
    int cz = (int)(z / CHUNK_SIZE);

    chunk* c = get_chunk(cx, cz);

    // Calculate coordinate within chunk
    *chunk_x = (int)(mod(x, CHUNK_SIZE));
    *chunk_z = (int)(mod(z, CHUNK_SIZE));

    return c;
}
