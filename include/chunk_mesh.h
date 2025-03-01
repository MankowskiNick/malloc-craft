#ifndef CHUNK_MESH_H
#define CHUNK_MESH_H

#include <camera.h>

#define VBO_WIDTH 7
#define SIDE_OFFSET VERTS_PER_SIDE * VBO_WIDTH

typedef struct {
    float x, y, z;
    float tx, ty;
    float atlas_x, atlas_y;
} side_vertex;

typedef struct {
    side_vertex vertices[6];
} side_data;

typedef struct {
    int x, y, z;
    int atlas_x, atlas_y;
    short side;
} side_instance;

typedef struct {
    int x, z;
    side_instance* opaque_sides;
    side_instance* transparent_sides;
    int num_opaque_sides;
    int num_transparent_sides;

    // float* transparent_data;
    // float* opaque_data;
    int* transparent_data;
    int* opaque_data;
} chunk_mesh;

void chunk_mesh_init(camera* camera);
int chunk_mesh_equals(void* a, void* b);
// float* chunk_mesh_to_float_array_old(side_data* sides, int num_sides);
int* chunk_mesh_to_buffer(side_instance* sides, int num_sides);
void sort_transparent_sides(chunk_mesh* packet);

#endif