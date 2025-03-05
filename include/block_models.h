#ifndef MODELS_H
#define MODELS_H

#include <settings.h>
#include <util.h>

#define RIGHT  0.0f
#define LEFT   1.0f
#define BACK   2.0f
#define FRONT  3.0f
#define TOP    4.0f
#define BOTTOM 5.0f
#define UNKNOWN_SIDE -1.0f

enum {
    AIR,
    GRASS,
    DIRT,
    STONE,
    WEEZER,
    OAK_LOG,
    OAK_LEAF,
    WATER,
    GLASS,
    SAND,
    CACTUS,
    CACTUS_TOP,
    OAK_PLANKS
};

typedef struct {
    uint id;
    char* name;
    int transparent;
    int liquid;
    short face_atlas_coords[6][2];
} block_type;

extern block_type TYPES[];

typedef struct {
    int pos[3];
    block_type* type;
} block;

typedef struct {
    int x, z;
    short blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
} chunk;

#endif