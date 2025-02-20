#ifndef BLOCK_H
#define BLOCK_H

#include <block_types.h>
#include <settings.h>

#define LEFT   0.0f
#define RIGHT  1.0f
#define TOP    2.0f
#define BOTTOM 3.0f
#define FRONT  4.0f
#define BACK   5.0f

typedef struct {
    int pos[3];
    block_type* type;
} block;

// Vertex data for a cube
// v.x v.y v.z t.x t.y side
#define CUBE_VERTICES_WIDTH 6
extern float CUBE_VERTICES[];

int get_side_visible(uint side, block_type* blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE], int x, int y, int z);

#endif