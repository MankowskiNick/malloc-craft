#ifndef WORLD_MESH_H
#define WORLD_MESH_H

#include "chunk_mesh.h"


void wm_init(camera* camera);
world_mesh* create_world_mesh(chunk_mesh** packet, int count);
void get_world_mesh(mesh_args* args);
void free_world_mesh(world_mesh* mesh);

void lock_world_mesh();
void unlock_world_mesh();

#endif
