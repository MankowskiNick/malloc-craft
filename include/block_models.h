#ifndef MODELS_H
#define MODELS_H

#include <util/settings.h>
#include <util.h>

#define NORTH   0.0f
#define WEST    1.0f
#define SOUTH   2.0f
#define EAST    3.0f
#define UP      4.0f
#define DOWN    5.0f
#define UNKNOWN_SIDE 6.0f

// 3-byte block storage type
typedef struct {
    unsigned char bytes[3];
} block_data_t;

// Helper functions to convert between block_data_t and int
static inline int block_data_to_int(block_data_t bd) {
    return (int)(bd.bytes[0] | (bd.bytes[1] << 8) | (bd.bytes[2] << 16));
}

static inline block_data_t int_to_block_data(int data) {
    block_data_t bd;
    bd.bytes[0] = (unsigned char)(data & 0xFF);
    bd.bytes[1] = (unsigned char)((data >> 8) & 0xFF);
    bd.bytes[2] = (unsigned char)((data >> 16) & 0xFF);
    return bd;
}

typedef struct {
    uint id;
    char* name;
    int transparent;
    int liquid;
    int is_foliage;
    int is_custom_model;
    int oriented;
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
    block_data_t blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
} chunk;

#endif