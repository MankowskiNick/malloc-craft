#include <world.h>
#include <chunk.h>
#include <hashmap.h>


#include <stdlib.h>
#include <stdio.h>

#define CHUNK_CACHE_SIZE 2048

#define mod(x, y) fmod(x, y) < 0 ? fmod(x, y) + (y) : fmod(x,y)

typedef struct {
    int x, z;
} chunk_coord;

uint hash(chunk_coord c) {
    uint hash = (unsigned int)((c.x * 73856093) ^ (c.z * 19349663)) + CHUNK_CACHE_SIZE;
    return hash % CHUNK_CACHE_SIZE;
}

int equals(chunk_coord a, chunk_coord b) {
    return a.x == b.x && a.z == b.z;
}

DEFINE_HASHMAP(chunk_map, chunk_coord, chunk*, hash, equals);
typedef chunk_map_hashmap chunk_map;
chunk_map chunks;

void w_init() {
    c_init();

    chunks = chunk_map_init(CHUNK_CACHE_SIZE);
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

chunk* get_chunk_at(float x, float z, uint* chunk_x, uint* chunk_z) {
    
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