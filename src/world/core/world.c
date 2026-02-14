#include "world.h"
#include "chunk.h"
#include "chunk_io.h"
#include <hashmap.h>


#include <stdlib.h>
#include <stdio.h>

#define mod(x, y) fmod(x, y) < 0 ? fmod(x, y) + (y) : fmod(x,y)

#define WORLDS_DIR "./worlds/"

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
