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

typedef struct {
    int x;
    int z;
    int* num_packets;
} get_world_mesh_args;

void wm_init(camera* camera);
world_mesh* create_world_mesh(chunk_mesh** packet, int count);
world_mesh* get_world_mesh(get_world_mesh_args* args);
void free_world_mesh(world_mesh* mesh);

#endif