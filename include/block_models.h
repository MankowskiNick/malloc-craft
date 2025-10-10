#ifndef MODELS_H
#define MODELS_H

#include <settings.h>
#include <util.h>

#define SOUTH   0.0f
#define NORTH   1.0f
#define WEST    2.0f
#define EAST    3.0f
#define UP      4.0f
#define DOWN    5.0f
#define UNKNOWN_SIDE 6.0f

typedef struct {
    uint id;
    char* name;
    int transparent;
    int liquid;
    int is_foliage;
    int is_custom_model;
    int orientations[6];
    char* model;
    char* models[6];
    short face_atlas_coords[6][2];
} block_type;

extern block_type* TYPES;

typedef struct {
    int pos[3];
    block_type* type;
} block;

typedef struct {
    int x, z;
    short blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
} chunk;

#endif