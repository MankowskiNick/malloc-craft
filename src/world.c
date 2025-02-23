#include <world.h>
#include <chunk.h>

#include <stdlib.h>
#include <stdio.h>

#define CHUNK_CACHE_SIZE 2048

#define mod(x, y) fmod(x, y) < 0 ? fmod(x, y) + (y) : fmod(x,y)


typedef struct node {
    chunk* c;
    struct node* next;
} node;

node* world[CHUNK_CACHE_SIZE];

uint hash(int x, int z) {
    uint hash = (unsigned int)((x * 73856093) ^ (z * 19349663)) + CHUNK_CACHE_SIZE;
    return hash % CHUNK_CACHE_SIZE;
}

void free_node(node* n) {
    if (n->next != NULL) {
        free_node(n->next);
    }
    free(n->c);
    free(n);
}

void add_chunk(chunk* c) {
    uint index = hash(c->x, c->z);

    // cache contains chunk already
    if (world[index] != NULL) {
        node* cur = world[index];
        while (cur->next != NULL) {
            // chunk already in cache
            if (cur->c->x == c->x && cur->c->z == c->z) {
                return;
            }
            cur = cur->next;
        }
        node* n = malloc(sizeof(node));
        n->c = c;
        n->next = NULL;
        cur->next = n;
        return;
    }

    // cache does not contain chunk
    node* n = malloc(sizeof(node));
    n->c = c;
    n->next = NULL;
    world[index] = n;
}

void w_init() {
    for (int i = 0; i < CHUNK_CACHE_SIZE; i++) {
        world[i] = NULL;
    }
}

void w_cleanup() {
    for (int i = 0; i < CHUNK_CACHE_SIZE; i++) {
        if (world[i] != NULL) {
            free_node(world[i]);
        }
    }
}

chunk* get_chunk(int x, int z) {
    uint index = hash(x, z);
    node* cur = world[index];

    // look for chunk in cache
    while (cur != NULL) {
        if (cur->c->x == x && cur->c->z == z) {
            return cur->c;
        }
        cur = cur->next;
    }

    // chunk not in cache
    chunk* c = malloc(sizeof(chunk));
    chunk_create(c, x, z);
    add_chunk(c);
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