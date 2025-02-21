#ifndef MODELS_H
#define MODELS_H

#include <settings.h>
#include <util.h>

#define FRONT  0.0f
#define BACK   1.0f
#define LEFT   2.0f
#define RIGHT  3.0f
#define TOP    4.0f
#define BOTTOM 5.0f

typedef struct {
    uint id;
    char* name;
    float face_atlas_coords[6][2];
} block_type;
extern block_type TYPES[];

typedef struct {
    int pos[3];
    block_type* type;
} block;

typedef struct {
    int x, z;
    block_type* blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
} chunk;

#endif