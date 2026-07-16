#ifndef WORLD_MESH_H
#define WORLD_MESH_H

#include <chunk_mesh.h>
#include <pthread.h>

void init_world_mesh(camera* camera);
void get_world_mesh(game_data* args);
void start_world_mesh_updater(game_data* args);
void kill_world_mesh_updater(void);

#endif
