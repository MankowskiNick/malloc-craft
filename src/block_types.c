#include <block_types.h>

block_type TYPES[] = {
    {
        .id = 0,
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