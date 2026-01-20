#ifndef WORLD_MESH_H
#define WORLD_MESH_H

#include <chunk_mesh.h>
#include <pthread.h>

// Forward declaration - defined in mesh.h
struct world_mesh_buffer;
typedef struct world_mesh_buffer world_mesh_buffer;

void wm_init(camera* camera);
world_mesh* create_world_mesh(chunk_mesh** packet, int count);
world_mesh* create_world_mesh_into_buffer(chunk_mesh** packet, int count, world_mesh_buffer* buf);
void get_world_mesh(game_data* args);
void start_world_mesh_updater(game_data* args);
void free_world_mesh(world_mesh* mesh);
void wm_cleanup(void);

#endif
