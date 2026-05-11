#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <player/core/player.h>
#include <util.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define FLOATS_PER_MODEL_VERT 8

typedef struct {
    int x, y, z;
    short atlas_x, atlas_y;
    short side;
    short face;
    bool underwater;
    short water_level;  // 0 = dry, 1-7 = flowing water (7=strongest), 8 = source
    short orientation;
    short water_level_transition;  // For water flow: adjacent water level (height we're transitioning FROM)
    int ao;  // Packed AO values for 4 vertices (2 bits each, values 0-3)
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

static inline void free_world_mesh(world_mesh* mesh) {
    if (mesh == NULL) {
        return;
    }
    free(mesh->transparent_data);
    free(mesh->opaque_data);
    free(mesh->liquid_data);
    if (mesh->num_foliage_sides > 0) {
        free(mesh->foliage_data);
    }
    if (mesh->num_custom_verts > 0) {
        free(mesh->custom_model_data);
    }
    free(mesh);
}

typedef struct double_buffer_t {
    
    // buffers;
    world_mesh* a;
    world_mesh* b;
    int a_count, b_count;
    pthread_mutex_t a_lock, b_lock;

    // t = a, 
    // f = b
    bool read_buffer;
} double_buffer_t;

static inline double_buffer_t* create_double_buffer() {
    double_buffer_t* buf = malloc(sizeof(double_buffer_t));

    buf->read_buffer = 0;
    buf->a = NULL;
    buf->b = NULL;

    pthread_mutex_init(&buf->a_lock, NULL);
    pthread_mutex_init(&buf->a_lock, NULL);

    return buf;
}

static inline void lock_double_buffer(double_buffer_t* buf, bool target_buffer) {
    if (buf == NULL) {
        printf("ERROR: Failed to lock double buffer.  Argument \"buf\" cannot be NULL.\n");
        return;
    }

    if (target_buffer) {
        pthread_mutex_lock(&buf->a_lock);
    }
    else {
        pthread_mutex_lock(&buf->b_lock);
    }
}

static inline void unlock_double_buffer(double_buffer_t* buf, bool target_buffer) {
    if (buf == NULL) {
        printf("ERROR: Failed to lock double buffer.  Argument \"buf\" cannot be NULL.\n");
        return;
    }

    if (target_buffer) {
        pthread_mutex_unlock(&buf->a_lock);
    }
    else {
        pthread_mutex_unlock(&buf->b_lock);
    }
}

static inline void write_double_buffer(double_buffer_t* buf, world_mesh* new) {
    world_mesh** addr = NULL;

    if (buf->read_buffer) {
        addr = &buf->a;
    }
    else {
        addr = &buf->b;
    }

    bool target = buf->read_buffer;
    buf->read_buffer = !buf->read_buffer;
    
    lock_double_buffer(buf, target);
    world_mesh* old = *addr;
    *addr = new;
    free_world_mesh(old);
    unlock_double_buffer(buf, target);
}

typedef struct {
    int x;
    int z;
    int* num_packets;
    chunk_mesh** packet;
    double_buffer_t* world_mesh;
    player player;
    int is_running;
    bool mesh_requires_update;

    float tick;

    // UI state
    bool show_fps;
    bool mesh_refresh_paused;  // Pause mesh refresh updates (F2 toggle)
    int fps;
} game_data;

static inline void destroy_game_data(game_data data) {
    free(data.world_mesh);
    free(data.packet);
}

static inline void update_game_data(game_data* data) {
    data->x = data->player.cam.position[0];
    data->z = data->player.cam.position[2];
}

static inline world_mesh* copy_world_mesh(double_buffer_t* buf) {
    world_mesh* src = NULL;
    
    bool target = buf->read_buffer;
    lock_double_buffer(buf, target);
    if (buf->read_buffer) {
        src = buf->a;
    } else {
        src = buf->b;
    }

    buf->read_buffer = !buf->read_buffer;
    
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
        size_t custom_model_size = sizeof(float) * FLOATS_PER_MODEL_VERT * src->num_custom_verts;
        copy->custom_model_data = (float*)malloc(custom_model_size);
        memcpy(copy->custom_model_data, src->custom_model_data, custom_model_size);
    } else {
        copy->custom_model_data = NULL;
    }

    unlock_double_buffer(buf, target);
    
    return copy;
}

#endif
