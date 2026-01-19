#include "world.h"
#include "chunk.h"
#include <hashmap.h>


#include <stdlib.h>
#include <stdio.h>

#define mod(x, y) fmod(x, y) < 0 ? fmod(x, y) + (y) : fmod(x,y)

DEFINE_HASHMAP(chunk_map, chunk_coord, chunk*, chunk_hash, chunk_equals);
typedef chunk_map_hashmap chunk_map;
chunk_map chunks;

void w_init() {
    c_init();

    chunks = chunk_map_init(CHUNK_CACHE_SIZE);
}

void w_cleanup() {
    chunk_map_free(&chunks);
}

chunk* get_chunk(int x, int z) {
    chunk_coord coord = {x, z};
    chunk** found = chunk_map_get(&chunks, coord);
    chunk* c = found ? *found : NULL;
    if (c == NULL) {
        c = malloc(sizeof(chunk));
        chunk_create(c, x, z);
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
