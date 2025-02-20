#ifndef CHUNK_H
#define CHUNK_H

#include <settings.h>
#include <block.h>

typedef struct {
    int x, z;
    block_type* blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
} chunk;

#endif