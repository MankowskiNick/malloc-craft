#include <world.h>
#include <stdlib.h>

#include <stdio.h>
#include <chunk.h>

#define CHUNK_CACHE_SIZE 2048


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
    c_init(c, x, z);
    add_chunk(c);
    return c;
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