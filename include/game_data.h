#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <player/core/player.h>
#include <util.h>
#include <string.h>
#include <stdlib.h>

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
    short lod_scale;
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

    float tick;

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
    free(data.player);
}

static inline world_mesh* copy_world_mesh(world_mesh* src) {
    if (!src) return NULL;
    
    world_mesh* copy = (world_mesh*)malloc(sizeof(world_mesh));
    copy->num_opaque_sides = src->num_opaque_sides;
    copy->num_transparent_sides = src->num_transparent_sides;
    copy->num_liquid_sides = src->num_liquid_sides;
    copy->num_foliage_sides = src->num_foliage_sides;
    copy->num_custom_verts = src->num_custom_verts;
    
    if (src->opaque_data && src->num_opaque_sides > 0) {
        int opaque_size = src->num_opaque_sides * VBO_WIDTH;
        copy->opaque_data = (int*)malloc(sizeof(int) * opaque_size);
        memcpy(copy->opaque_data, src->opaque_data, sizeof(int) * opaque_size);
    } else {
        copy->opaque_data = NULL;
    }
    
    if (src->transparent_data && src->num_transparent_sides > 0) {
        int transparent_size = src->num_transparent_sides * VBO_WIDTH;
        copy->transparent_data = (int*)malloc(sizeof(int) * transparent_size);
        memcpy(copy->transparent_data, src->transparent_data, sizeof(int) * transparent_size);
    } else {
        copy->transparent_data = NULL;
    }
    
    if (src->liquid_data && src->num_liquid_sides > 0) {
        int liquid_size = src->num_liquid_sides * VBO_WIDTH;
        copy->liquid_data = (int*)malloc(sizeof(int) * liquid_size);
        memcpy(copy->liquid_data, src->liquid_data, sizeof(int) * liquid_size);
    } else {
        copy->liquid_data = NULL;
    }
    
    if (src->foliage_data && src->num_foliage_sides > 0) {
        int foliage_size = src->num_foliage_sides * VBO_WIDTH;
        copy->foliage_data = (int*)malloc(sizeof(int) * foliage_size);
        memcpy(copy->foliage_data, src->foliage_data, sizeof(int) * foliage_size);
    } else {
        copy->foliage_data = NULL;
    }
    
    if (src->custom_model_data && src->num_custom_verts > 0) {
        copy->custom_model_data = (float*)malloc(sizeof(float) * src->num_custom_verts);
        memcpy(copy->custom_model_data, src->custom_model_data, sizeof(float) * src->num_custom_verts);
    } else {
        copy->custom_model_data = NULL;
    }
    
    return copy;
}

#endif