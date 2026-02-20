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
#define FLOATS_PER_MODEL_VERT 8

// Double-buffered packet storage for chunk meshes
// Allows workers to build in staging buffer while render thread reads from active
typedef struct chunk_mesh_builder {
    chunk_mesh** mesh_array;
    int capacity;
    int count;
} chunk_mesh_builder;

// Pre-allocated world mesh buffers for double-buffering
typedef struct world_mesh_buffer {
    int* transparent_data;
    int* opaque_data;
    int* liquid_data;
    int* foliage_data;
    float* custom_model_data;
    
    // Counts for current frame
    int num_transparent_sides;
    int num_opaque_sides;
    int num_liquid_sides;
    int num_foliage_sides;
    int num_custom_verts;
    
    // Capacity tracking for safe reuse
    int transparent_capacity;
    int opaque_capacity;
    int liquid_capacity;
    int foliage_capacity;
    int custom_capacity;
} world_mesh_buffer;

// Chunk mesh builder functions
chunk_mesh_builder* builder_init(int initial_capacity);
void builder_add_mesh(chunk_mesh_builder* b, chunk_mesh* mesh);
void builder_clear(chunk_mesh_builder* b);
void builder_cleanup(chunk_mesh_builder* b);

// World mesh buffer functions
world_mesh_buffer* wm_buffer_init(int initial_transparent, int initial_opaque,
                                   int initial_liquid, int initial_foliage,
                                   int initial_custom);
void wm_buffer_ensure_capacity(world_mesh_buffer* buf, int transparent_needed,
                                int opaque_needed, int liquid_needed,
                                int foliage_needed, int custom_needed);
void wm_buffer_reset_counts(world_mesh_buffer* buf);
void wm_buffer_free(world_mesh_buffer* buf);

void m_init(camera* camera);
void m_cleanup();
void preload_initial_chunks(game_data* data);
chunk_mesh* update_chunk_mesh(int x, int z, float player_x, float player_z);
chunk_mesh* get_chunk_mesh(int x, int z);
void invalidate_chunk_mesh_all_lods(int x, int z);
void queue_chunk_for_sorting(chunk_mesh* packet, int px, int py);
void sort_chunk();
void load_chunk(float player_x, float player_z);
void wait_chunk_loading(void);
short get_adjacent_block_id(int x, int y, int z, short side, chunk* c, chunk* adj);
block_data_t get_block_data(int x, int y, int z, chunk* c);
short calculate_lod(int x, int z, float player_x, float player_z);
int is_chunk_in_foliage_distance(int chunk_x, int chunk_z, float player_x, float player_z);
int is_chunk_in_transparent_distance(int chunk_x, int chunk_z, float player_x, float player_z);
void process_chunk_work_item(chunk_work_item* work);

#endif
