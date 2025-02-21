#ifndef CHUNK_H
#define CHUNK_H

#include <settings.h>
#include <block.h>

typedef struct {
    int x, z;
    block_type* blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
} chunk;

void c_init(chunk* c, int x, int z);

int get_side_visible(camera cam, int x, int y, int z, uint side, chunk* c, chunk* adj);

#endif