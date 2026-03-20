#ifndef MESH_H
#define MESH_H

#include <chunk.h>
#include <hashmap.h>
#include <chunk_mesh.h>
#include <block_models.h>
#include <game_data.h>
#include "worker_pool.h"
#include <pthread.h>

#define SIDES_PER_CHUNK 1000
#define MODEL_VERTICES_PER_CHUNK 1000

void m_init(camera* camera);
void mesh_cleanup();
void preload_initial_chunks(game_data* data);
chunk_mesh* update_chunk_mesh(int x, int z, float player_x, float player_z);
chunk_mesh* get_chunk_mesh(int x, int z);
void invalidate_chunk_mesh_all_lods(int x, int z);
void queue_chunk_for_sorting(chunk_mesh* packet, int px, int py);
void sort_chunk();
void load_chunk(float player_x, float player_z);
void wait_chunk_loading(void);
block_data_t get_block_data(int x, int y, int z, chunk* c);
short calculate_lod(int x, int z, float player_x, float player_z);
int is_chunk_in_foliage_distance(int chunk_x, int chunk_z, float player_x, float player_z);
int is_chunk_in_transparent_distance(int chunk_x, int chunk_z, float player_x, float player_z);
void process_chunk_work_item(chunk_work_item* work);
chunk_mesh* create_chunk_mesh(int x, int z, float player_x, float player_z);
void get_mesh_player_pos(float* out_x, float* out_z);

#endif
