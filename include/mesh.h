#ifndef MESH_H
#define MESH_H

#include <chunk.h>
#include <hashmap.h>

#define VBO_WIDTH 7
#define VERTS_PER_SIDE 6
#define SIDE_OFFSET VERTS_PER_SIDE * VBO_WIDTH
#define INITIAL_VBO_SIZE 5000 * SIDE_OFFSET

typedef struct {
    float* opaque_side_data;
    float* transparent_side_data;
    int num_opaque_sides;
    int num_transparent_sides;
} chunk_mesh;

void m_init();
void update_chunk_mesh(int x, int z);
chunk_mesh* get_chunk_mesh(int x, int z);

#endif