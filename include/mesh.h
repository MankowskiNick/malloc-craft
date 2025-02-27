#ifndef MESH_H
#define MESH_H

#include <chunk.h>
#include <hashmap.h>

#define VBO_WIDTH 7
#define VERTS_PER_SIDE 6
#define SIDE_OFFSET VERTS_PER_SIDE * VBO_WIDTH
#define SIDES_PER_CHUNK 1000

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


void m_init(camera* camera);
void m_cleanup();
void update_chunk_mesh(int x, int z);
chunk_mesh* get_chunk_mesh(int x, int z);
void sort_transparent_sides(chunk_mesh* packet);

#endif