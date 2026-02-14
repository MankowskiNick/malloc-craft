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

camera_cache wm_camera_cache = {0, 0, 0};

// Deep copy a chunk_mesh and all its dynamically allocated data
static chunk_mesh* copy_chunk_mesh(chunk_mesh* src) {
    if (src == NULL) {
        return NULL;
    }
    
    chunk_mesh* dst = malloc(sizeof(chunk_mesh));
    assert(dst != NULL && "ERROR: Could not allocate memory for chunk_mesh copy.\n");
    
    // Copy structure fields
    dst->x = src->x;
    dst->z = src->z;
    dst->num_opaque_sides = src->num_opaque_sides;
    dst->num_transparent_sides = src->num_transparent_sides;
    dst->num_liquid_sides = src->num_liquid_sides;
    dst->num_foliage_sides = src->num_foliage_sides;
    dst->num_custom_verts = src->num_custom_verts;
    dst->lod_scale = src->lod_scale;
    
    // Deep copy dynamic arrays
    if (src->opaque_sides != NULL && src->num_opaque_sides > 0) {
        dst->opaque_sides = malloc(src->num_opaque_sides * sizeof(side_instance));
        assert(dst->opaque_sides != NULL && "ERROR: Could not allocate memory for opaque_sides copy.\n");
        memcpy(dst->opaque_sides, src->opaque_sides, src->num_opaque_sides * sizeof(side_instance));
    } else {
        dst->opaque_sides = NULL;
    }
    
    if (src->transparent_sides != NULL && src->num_transparent_sides > 0) {
        dst->transparent_sides = malloc(src->num_transparent_sides * sizeof(side_instance));
        assert(dst->transparent_sides != NULL && "ERROR: Could not allocate memory for transparent_sides copy.\n");
        memcpy(dst->transparent_sides, src->transparent_sides, src->num_transparent_sides * sizeof(side_instance));
    } else {
        dst->transparent_sides = NULL;
    }
    
    if (src->liquid_sides != NULL && src->num_liquid_sides > 0) {
        dst->liquid_sides = malloc(src->num_liquid_sides * sizeof(side_instance));
        assert(dst->liquid_sides != NULL && "ERROR: Could not allocate memory for liquid_sides copy.\n");
        memcpy(dst->liquid_sides, src->liquid_sides, src->num_liquid_sides * sizeof(side_instance));
    } else {
        dst->liquid_sides = NULL;
    }
    
    if (src->foliage_sides != NULL && src->num_foliage_sides > 0) {
        dst->foliage_sides = malloc(src->num_foliage_sides * sizeof(side_instance));
        assert(dst->foliage_sides != NULL && "ERROR: Could not allocate memory for foliage_sides copy.\n");
        memcpy(dst->foliage_sides, src->foliage_sides, src->num_foliage_sides * sizeof(side_instance));
    } else {
        dst->foliage_sides = NULL;
    }
    
    if (src->custom_model_data != NULL && src->num_custom_verts > 0) {
        dst->custom_model_data = malloc(src->num_custom_verts * FLOATS_PER_MODEL_VERT * sizeof(float));
        assert(dst->custom_model_data != NULL && "ERROR: Could not allocate memory for custom_model_data copy.\n");
        memcpy(dst->custom_model_data, src->custom_model_data, src->num_custom_verts * FLOATS_PER_MODEL_VERT * sizeof(float));
    } else {
        dst->custom_model_data = NULL;
    }
    
    return dst;
}

void wm_init(camera* camera) {
    wm_camera_cache.x = camera->position[0];
    wm_camera_cache.y = camera->position[1];
    wm_camera_cache.z = camera->position[2];
    chunk_mesh_init(camera);
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
        if (mesh == NULL) {
            continue;
        }
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

        if (mesh == NULL) {
            continue;
        }

        if (mesh->transparent_sides != NULL && mesh->num_transparent_sides > 0) {
            chunk_mesh_to_buffer(transparent_data + transparent_offset, 
                mesh->transparent_sides, 
                mesh->num_transparent_sides,
                mesh->lod_scale);
            transparent_offset += mesh->num_transparent_sides * VBO_WIDTH;
        }
        
        if (mesh->opaque_sides != NULL && mesh->num_opaque_sides > 0) {
            chunk_mesh_to_buffer(opaque_data + opaque_offset, 
                mesh->opaque_sides, 
                mesh->num_opaque_sides,
                mesh->lod_scale);
            opaque_offset += mesh->num_opaque_sides * VBO_WIDTH;
        }
        
        if (mesh->liquid_sides != NULL && mesh->num_liquid_sides > 0) {
            chunk_mesh_to_buffer(liquid_data + liquid_offset, 
                mesh->liquid_sides, 
                mesh->num_liquid_sides,
                mesh->lod_scale);
            liquid_offset += mesh->num_liquid_sides * VBO_WIDTH;
        }

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

    chunk_mesh** packet = malloc(*(args->num_packets) * sizeof(chunk_mesh*));
    assert(packet != NULL && "ERROR: Could not allocate memory for chunk_mesh double buffer in world_mesh generation.\n");

    lock_mesh();
    
    // Deep copy packet list while holding lock to avoid data races
    int packet_count = *(args->num_packets);
    for (int i = 0; i < packet_count; i++) {
        packet[i] = copy_chunk_mesh(args->packet[i]);
    }

    unlock_mesh();

    // Create world mesh outside lock with copied packet data
    world_mesh* world = create_world_mesh(packet, packet_count);

    if (!world) {
        assert(false && "Failed to create world mesh\n");
    }

    lock_mesh();
    
    // Free the original chunk meshes
    if (args->world_mesh != NULL) {
        free_world_mesh(args->world_mesh);
    }
    args->world_mesh = world;
    args->mesh_requires_update = FALSE;

    unlock_mesh();
    
    // Free copied chunk_mesh structures and their data
    for (int i = 0; i < packet_count; i++) {
        if (packet[i] != NULL) {
            chunk_mesh* p = packet[i];
            free(p->opaque_sides);
            free(p->transparent_sides);
            free(p->liquid_sides);
            free(p->foliage_sides);
            free(p->custom_model_data);
            free(p);
        }
    }
    free(packet);
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
    pthread_mutex_destroy(&wm_mutex);
}
