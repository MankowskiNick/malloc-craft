#ifndef CHUNK_H
#define CHUNK_H

#include <settings.h>
#include <block.h>

typedef struct {
    uint id;
    int x, z;
    block_type* blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
} chunk;

void c_init(chunk* c, int x, int z);

int get_side_visible(int x, int y, int z, uint side, chunk* c, chunk* adj);//, block_type* blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE]);//, block_type* adjacent_blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE]);


#endif