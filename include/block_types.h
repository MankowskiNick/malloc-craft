#ifndef BLOCK_TYPES
#define BLOCK_TYPES



#include <util.h>

typedef struct {
    uint id;
    bool transparent;
    char* name;
    float face_atlas_coords[6][2];
} block_type;

block_type TYPES[] = {
    {
        .id = 0,
        .transparent = 1,
        .name = "air",
        .face_atlas_coords = {
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
        }
    },
    {
        .id = 1,
        .transparent = 0,
        .name = "grass",
        .face_atlas_coords = {
            {1.0f / 32.0f, 0.0f},
            {1.0f / 32.0f, 0.0f},
            {0.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
            {1.0f / 32.0f, 0.0f},
            {1.0f / 32.0f, 0.0f}
        }
    },
    {
        .id = 2,
        .transparent = 0,
        .name = "dirt",
        .face_atlas_coords = {
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f}
        }
    },
    {
        .id = 3,
        .transparent = 0,
        .name = "stone",
        .face_atlas_coords = {
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f}
        }
    }
};

#endif