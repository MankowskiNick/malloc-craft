#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <player.h>

typedef struct {
    int x, y, z;
    short atlas_x, atlas_y;
    short side;
    short face;
    bool underwater;
    short water_level;  // 0 = dry, 1-7 = flowing water (7=strongest), 8 = source
    short orientation;
    short water_level_transition;  // For water flow: adjacent water level (height we're transitioning FROM)
} side_instance;

typedef struct {
    int x, z;
    side_instance* opaque_sides;
    side_instance* liquid_sides;
    side_instance* transparent_sides;
    side_instance* foliage_sides;
    float* custom_model_data;
    int num_opaque_sides;
    int num_transparent_sides;
    int num_liquid_sides;
    int num_foliage_sides;
    int num_custom_verts;
} chunk_mesh;

typedef struct {
    int num_opaque_sides;
    int num_transparent_sides;
    int num_liquid_sides;
    int num_foliage_sides;
    int num_custom_verts;

    int* transparent_data;
    int* opaque_data;
    int* liquid_data;
    int* foliage_data;
    float* custom_model_data;
} world_mesh;

typedef struct {
    int x;
    int z;
    int* num_packets;
    chunk_mesh** packet;
    world_mesh* world_mesh;
    player* player;
    int is_running;
    bool mesh_requires_update;

    // array of chunks that need water updates.  this is an array of chunk ids determined by a prime factorization hash.
    int* chunks_to_flow;
    int num_chunks_to_flow;

    int tick;

    // UI state
    bool show_fps;
    int fps;
    
    // FPS averaging
    float* frame_time_buffer;
    int frame_buffer_index;
    int fps_average_frames;
    int average_fps;
} game_data;

static inline void free_game_data(game_data data) {
    free(data.chunks_to_flow);
    free(data.frame_time_buffer);
    free(data.world_mesh);
    free(data.packet);
}

#endif