#ifndef CHUNK_MESH_H
#define CHUNK_MESH_H

#include <camera.h>
#include <player_instance.h>
#include <game_data.h>
#include <util.h>

#define VBO_WIDTH 9

void chunk_mesh_init(camera* camera);
int chunk_mesh_equals(void* a, void* b);
void chunk_mesh_to_buffer(int* head, side_instance* sides, int num_sides);
void custom_vert_to_buffer(float* head, float* custom_verts, int num_custom_verts);
void sort_transparent_sides(chunk_mesh* packet);
void sort_liquid_sides(chunk_mesh* packet);
void get_chunk_meshes(game_data* args);
void start_chunk_mesh_updater(game_data* args);

void lock_mesh();
void unlock_mesh();

#endif
