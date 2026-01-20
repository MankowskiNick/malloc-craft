#ifndef CHUNK_MESH_H
#define CHUNK_MESH_H

#include <player/core/camera.h>
#include <player/core/player.h>
#include <game_data.h>
#include <util.h>

#define VBO_WIDTH 11

// Cache key that includes both chunk coordinate and LOD level
// This allows caching multiple LOD versions of the same chunk
typedef struct {
    int x, z;
    short lod;
} chunk_mesh_key;

// Work item for chunk mesh generation in worker pool
typedef struct {
    int x, z;
    float player_x, player_z;
    chunk_mesh* result_mesh;
    int work_complete;
} chunk_work_item;

// Hash and equality functions for LOD-aware cache key
uint chunk_mesh_key_hash(chunk_mesh_key k);
int chunk_mesh_key_equals(chunk_mesh_key a, chunk_mesh_key b);

void chunk_mesh_init(camera* camera);
int chunk_mesh_equals(void* a, void* b);
int chunk_work_item_equals(void* a, void* b);
void chunk_mesh_to_buffer(int* head, side_instance* sides, int num_sides, int lod_scale);
void custom_vert_to_buffer(float* head, float* custom_verts, int num_custom_verts);
void sort_transparent_sides(chunk_mesh* packet);
void sort_liquid_sides(chunk_mesh* packet);
void get_chunk_meshes(game_data* args);
void start_chunk_mesh_updater(game_data* args);

void lock_mesh();
void unlock_mesh();

#endif
