#ifndef CHUNK_MESH_H
#define CHUNK_MESH_H

typedef struct {
    float x, y, z;
    float tx, ty;
    float atlas_x, atlas_y;
} side_vertex;

typedef struct {
    side_vertex vertices[6];
} side_data;

typedef struct {
    int x, z;
    side_data* opaque_sides;
    side_data* transparent_sides;
    int num_opaque_sides;
    int num_transparent_sides;

    float* transparent_data;
    float* opaque_data;
} chunk_mesh;

#endif