#ifndef BLOCK_TYPES
#define BLOCK_TYPES

#include <util.h>

typedef struct {
    uint id;
    char* name;
    float face_atlas_coords[6][2];
} block_type;

extern block_type TYPES[];

#endif