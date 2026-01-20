#include "world_mesh.h"
#include "util/settings.h"
#include <assert.h>
#include <string.h>
#include "util/sort.h"
#include <util.h>
#include "mesh.h"
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t wm_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t wm_swap_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wm_update_signal = PTHREAD_COND_INITIALIZER;

// Double-buffered world mesh buffers
static world_mesh_buffer* wm_buffer_active = NULL;
static world_mesh_buffer* wm_buffer_staging = NULL;

camera_cache wm_camera_cache = {0, 0, 0};

void wm_init(camera* camera) {
    wm_camera_cache.x = camera->position[0];
    wm_camera_cache.y = camera->position[1];
    wm_camera_cache.z = camera->position[2];
    chunk_mesh_init(camera);
    
    // Initialize double-buffered world mesh buffers
    // Start with capacity for chunks at render distance 8 (rough estimate: ~200 chunks)
    // Each chunk can have up to 1000 sides * VBO_WIDTH ints + 1000 custom verts * 8 floats
    int estimated_total_transparent = 1000000;   // ~1M ints
    int estimated_total_opaque = 1000000;
    int estimated_total_liquid = 500000;         // Typically fewer
    int estimated_total_foliage = 500000;
    int estimated_total_custom = 200000;         // Fewer custom models
    
    wm_buffer_active = wm_buffer_init(estimated_total_transparent, estimated_total_opaque,
                                       estimated_total_liquid, estimated_total_foliage,
                                       estimated_total_custom);
    wm_buffer_staging = wm_buffer_init(estimated_total_transparent, estimated_total_opaque,
                                        estimated_total_liquid, estimated_total_foliage,
                                        estimated_total_custom);
    
    if (!wm_buffer_active || !wm_buffer_staging) {
        fprintf(stderr, "Failed to initialize world mesh buffers\n");
        exit(EXIT_FAILURE);
    }
}

world_mesh* create_world_mesh(chunk_mesh** packet, int count) {
    assert(packet != NULL && "Chunk mesh pointer is NULL\n");
    assert(count > 0 && "Count must be greater than zero\n");

    // First pass: Calculate total memory needed
    int total_transparent_sides = 0;
    int total_foliage_sides = 0;
    int total_opaque_sides = 0;
    int total_liquid_sides = 0;
    int total_custom_verts = 0;
    for (int i = 0; i < count; i++) {
        chunk_mesh* mesh = packet[i];
        total_transparent_sides += mesh->num_transparent_sides;
        total_opaque_sides += mesh->num_opaque_sides;
        total_liquid_sides += mesh->num_liquid_sides;
        total_foliage_sides += mesh->num_foliage_sides;
        total_custom_verts += mesh->num_custom_verts;
    }

    // Allocate all memory at once
    int* transparent_data = malloc(total_transparent_sides * VBO_WIDTH * sizeof(int));
    int* opaque_data = malloc(total_opaque_sides * VBO_WIDTH * sizeof(int));
    int* liquid_data = malloc(total_liquid_sides * VBO_WIDTH * sizeof(int));
    int* foliage_data = malloc(total_foliage_sides * VBO_WIDTH * sizeof(int));
    float* custom_model_data = malloc(total_custom_verts * sizeof(float) * FLOATS_PER_MODEL_VERT);

    // Check for allocation failure
    if (!transparent_data || !opaque_data || !liquid_data || !foliage_data || !custom_model_data) {
        free(transparent_data);
        free(opaque_data);
        free(liquid_data);
        free(foliage_data);
        free(custom_model_data);
        assert(false && "Failed to allocate memory for world mesh data\n");
    }

    // Second pass: Copy data
    int transparent_offset = 0;
    int opaque_offset = 0;
    int liquid_offset = 0;
    int foliage_offset = 0;
    int custom_model_offset = 0;
    for (int i = 0; i < count; i++) {
        chunk_mesh* mesh = packet[i];

        if (mesh == NULL // this is a temp fix for a what the fuck bug
            || mesh->opaque_sides == NULL 
            || mesh->transparent_sides == NULL 
            || mesh->liquid_sides == NULL 
            || mesh->foliage_sides == NULL
            || mesh->custom_model_data == NULL) {
            continue;
        }

        chunk_mesh_to_buffer(transparent_data + transparent_offset, 
            mesh->transparent_sides, 
            mesh->num_transparent_sides,
            mesh->lod_scale);
        transparent_offset += mesh->num_transparent_sides * VBO_WIDTH;
        
        chunk_mesh_to_buffer(opaque_data + opaque_offset, 
            mesh->opaque_sides, 
            mesh->num_opaque_sides,
            mesh->lod_scale);
        opaque_offset += mesh->num_opaque_sides * VBO_WIDTH;
        
        chunk_mesh_to_buffer(liquid_data + liquid_offset, 
            mesh->liquid_sides, 
            mesh->num_liquid_sides,
            mesh->lod_scale);
        liquid_offset += mesh->num_liquid_sides * VBO_WIDTH;

        chunk_mesh_to_buffer(foliage_data + foliage_offset,
            mesh->foliage_sides,
            mesh->num_foliage_sides,
            mesh->lod_scale);
        foliage_offset += mesh->num_foliage_sides * VBO_WIDTH;

        custom_vert_to_buffer(custom_model_data + custom_model_offset, 
            mesh->custom_model_data, 
            mesh->num_custom_verts);
        custom_model_offset += mesh->num_custom_verts * FLOATS_PER_MODEL_VERT;
    }

    // Create the world mesh structure
    world_mesh* world = malloc(sizeof(world_mesh));
    if (!world) {
        free(transparent_data);
        free(opaque_data);
        free(liquid_data);
        free(foliage_data);
        free(custom_model_data);
        assert(false && "Failed to allocate memory for world mesh structure\n");
    }

    // Assign the data and counts to the world mesh
    world->transparent_data = transparent_data;
    world->opaque_data = opaque_data;
    world->liquid_data = liquid_data;
    world->foliage_data = foliage_data;
    world->num_transparent_sides = transparent_offset / VBO_WIDTH;
    world->num_opaque_sides = opaque_offset / VBO_WIDTH;
    world->num_liquid_sides = liquid_offset / VBO_WIDTH;
    world->num_foliage_sides = foliage_offset / VBO_WIDTH;
    world->custom_model_data = custom_model_data;
    world->num_custom_verts = custom_model_offset / FLOATS_PER_MODEL_VERT;

    return world;
}

void free_world_mesh(world_mesh* mesh) {
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

void get_world_mesh(game_data* args) {
    int x = args->x;
    int z = args->z;

    if (args->world_mesh != NULL) {
        int movedBlocks = ((int)x == (int)(wm_camera_cache.x) && (int)z == (int)(wm_camera_cache.z)) ? 0 : 1;
        if (movedBlocks || args->mesh_requires_update) {
            wm_camera_cache.x = x;
            wm_camera_cache.z = z;
        } else {
            // No need to update if the camera hasn't moved
            return;
        }
    }

    lock_mesh();

    world_mesh* world = create_world_mesh(args->packet, *args->num_packets);

    if (!world) {
        assert(false && "Failed to create world mesh\n");
    }

    // Free the original chunk meshes
    if (args->world_mesh != NULL) {
        free_world_mesh(args->world_mesh);
    }
    args->world_mesh = world;
    args->mesh_requires_update = FALSE;

    unlock_mesh();
}

void update_world_mesh(game_data* data) {
    if (data == NULL) {
        assert(false && "game_data pointer is NULL\n");
    }

    if (data->packet == NULL) {
        return;
    }

    while (data->is_running) {
        get_world_mesh(data);
        usleep(TICK_RATE);
    }

    return;
}

void start_world_mesh_updater(game_data* data) {
    pthread_t updater_thread;
    pthread_create(&updater_thread, NULL, (void* (*)(void*))update_world_mesh, data);
    pthread_detach(updater_thread);
}

void wm_cleanup(void) {
    if (wm_buffer_active != NULL) {
        wm_buffer_free(wm_buffer_active);
        wm_buffer_active = NULL;
    }
    if (wm_buffer_staging != NULL) {
        wm_buffer_free(wm_buffer_staging);
        wm_buffer_staging = NULL;
    }
    pthread_mutex_destroy(&wm_swap_mutex);
    pthread_cond_destroy(&wm_update_signal);
}

/**
 * Create world mesh into pre-allocated staging buffer
 * Optimized for minimal lock contention - does all work in staging buffer
 * then atomically swaps with active buffer
 */
world_mesh* create_world_mesh_into_buffer(chunk_mesh** packet, int count, world_mesh_buffer* buf) {
    assert(packet != NULL && "Chunk mesh pointer is NULL\n");
    assert(count > 0 && "Count must be greater than zero\n");
    assert(buf != NULL && "Buffer pointer is NULL\n");
    
    // Single pass: Calculate size and ensure capacity
    int total_transparent_sides = 0;
    int total_foliage_sides = 0;
    int total_opaque_sides = 0;
    int total_liquid_sides = 0;
    int total_custom_verts = 0;
    
    for (int i = 0; i < count; i++) {
        chunk_mesh* mesh = packet[i];
        if (mesh == NULL) continue;
        
        total_transparent_sides += mesh->num_transparent_sides;
        total_opaque_sides += mesh->num_opaque_sides;
        total_liquid_sides += mesh->num_liquid_sides;
        total_foliage_sides += mesh->num_foliage_sides;
        total_custom_verts += mesh->num_custom_verts;
    }
    
    // Ensure buffer has enough capacity
    wm_buffer_ensure_capacity(buf, total_transparent_sides * VBO_WIDTH,
                              total_opaque_sides * VBO_WIDTH,
                              total_liquid_sides * VBO_WIDTH,
                              total_foliage_sides * VBO_WIDTH,
                              total_custom_verts * FLOATS_PER_MODEL_VERT);
    
    // Reset counts
    wm_buffer_reset_counts(buf);
    
    // Single pass: Copy data into buffer
    int transparent_offset = 0;
    int opaque_offset = 0;
    int liquid_offset = 0;
    int foliage_offset = 0;
    int custom_model_offset = 0;
    
    for (int i = 0; i < count; i++) {
        chunk_mesh* mesh = packet[i];
        
        if (mesh == NULL || mesh->opaque_sides == NULL || mesh->transparent_sides == NULL ||
            mesh->liquid_sides == NULL || mesh->foliage_sides == NULL || 
            mesh->custom_model_data == NULL) {
            continue;
        }
        
        if (mesh->num_transparent_sides > 0) {
            chunk_mesh_to_buffer(buf->transparent_data + transparent_offset,
                mesh->transparent_sides, mesh->num_transparent_sides, mesh->lod_scale);
            transparent_offset += mesh->num_transparent_sides * VBO_WIDTH;
        }
        
        if (mesh->num_opaque_sides > 0) {
            chunk_mesh_to_buffer(buf->opaque_data + opaque_offset,
                mesh->opaque_sides, mesh->num_opaque_sides, mesh->lod_scale);
            opaque_offset += mesh->num_opaque_sides * VBO_WIDTH;
        }
        
        if (mesh->num_liquid_sides > 0) {
            chunk_mesh_to_buffer(buf->liquid_data + liquid_offset,
                mesh->liquid_sides, mesh->num_liquid_sides, mesh->lod_scale);
            liquid_offset += mesh->num_liquid_sides * VBO_WIDTH;
        }
        
        if (mesh->num_foliage_sides > 0) {
            chunk_mesh_to_buffer(buf->foliage_data + foliage_offset,
                mesh->foliage_sides, mesh->num_foliage_sides, mesh->lod_scale);
            foliage_offset += mesh->num_foliage_sides * VBO_WIDTH;
        }
        
        if (mesh->num_custom_verts > 0) {
            custom_vert_to_buffer(buf->custom_model_data + custom_model_offset,
                mesh->custom_model_data, mesh->num_custom_verts);
            custom_model_offset += mesh->num_custom_verts * FLOATS_PER_MODEL_VERT;
        }
    }
    
    // Update counts in buffer
    buf->num_transparent_sides = transparent_offset / VBO_WIDTH;
    buf->num_opaque_sides = opaque_offset / VBO_WIDTH;
    buf->num_liquid_sides = liquid_offset / VBO_WIDTH;
    buf->num_foliage_sides = foliage_offset / VBO_WIDTH;
    buf->num_custom_verts = custom_model_offset / FLOATS_PER_MODEL_VERT;
    
    return NULL;  // Returns NULL - caller will swap buffers atomically
}
