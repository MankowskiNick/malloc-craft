#ifndef WORLD_MESH_H
#define WORLD_MESH_H

#include "chunk_mesh.h"

typedef struct {
    int num_opaque_sides;
    int num_transparent_sides;
    int num_liquid_sides;

    int* transparent_data;
    int* opaque_data;
    int* liquid_data;
} world_mesh;

world_mesh* create_world_mesh(chunk_mesh** packet, int count);
void free_world_mesh(world_mesh* mesh);

#endif