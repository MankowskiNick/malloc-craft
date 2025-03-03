#ifndef CHUNK_MESH_H
#define CHUNK_MESH_H

#include <camera.h>

#define VBO_WIDTH 7

typedef struct {
    int x, y, z;
    short atlas_x, atlas_y;
    short side;
    short face;
    short underwater;
} side_instance;

typedef struct {
    int x, z;
    side_instance* opaque_sides;
    side_instance* liquid_sides;
    side_instance* transparent_sides;
    int num_opaque_sides;
    int num_transparent_sides;
    int num_liquid_sides;

    int* transparent_data;
    int* opaque_data;
    int* liquid_data;
} chunk_mesh;

void chunk_mesh_init(camera* camera);
int chunk_mesh_equals(void* a, void* b);
int* chunk_mesh_to_buffer(side_instance* sides, int num_sides);
void sort_transparent_sides(chunk_mesh* packet);

#endif