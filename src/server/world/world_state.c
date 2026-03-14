#include "world_state.h"

#include "../../world/core/chunk.h"
#include "chunk_io.h"

#include <util/settings.h>
#include <hashmap.h>
#include <stdio.h>

pthread_mutex_t disk_lock;

void init_world_state(void) {
    pthread_mutex_init(&disk_lock, NULL);

    // Initialize the worlds directory
    if (init_worlds_directory(WORLDS_DIR) == -1) {
        printf("Warning: Failed to initialize worlds directory\n");
    }
}

chunk* load_chunk_state(pthread_mutex_t* disk_lock, int x, int z) {
    chunk* c = malloc(sizeof(chunk));
    c->x = x;
    c->z = z;

    pthread_mutex_lock(disk_lock);
    int chunk_read = chunk_load_from_disk(c, WORLDS_DIR);
    pthread_mutex_unlock(disk_lock);

    if (chunk_read == -1) {
        chunk_create(c, x, z);
    }
    return c;
}

void save_chunk_state(chunk* c) {
    chunk_save_to_disk(c, WORLDS_DIR);
}