#include <world_mesh.h>
#include <settings.h>
#include <assert.h>
#include <string.h>
#include <sort.h>
#include <util.h>
#include <mesh.h>
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t wm_mutex = PTHREAD_MUTEX_INITIALIZER;

camera_cache wm_camera_cache = {0, 0, 0};


void wm_init(camera* camera) {
    wm_camera_cache.x = camera->position[0];
    wm_camera_cache.y = camera->position[1];
    wm_camera_cache.z = camera->position[2];
    chunk_mesh_init(camera);
}

world_mesh* create_world_mesh(chunk_mesh** packet, int count) {
    // First pass: Calculate total memory needed
    int total_transparent_sides = 0;
    int total_opaque_sides = 0;
    int total_liquid_sides = 0;
    for (int i = 0; i < count; i++) {
        chunk_mesh* mesh = packet[i];
        total_transparent_sides += mesh->num_transparent_sides;
        total_opaque_sides += mesh->num_opaque_sides;
        total_liquid_sides += mesh->num_liquid_sides;
    }

    // Allocate all memory at once
    int* transparent_data = malloc(total_transparent_sides * VBO_WIDTH * sizeof(int));
    int* opaque_data = malloc(total_opaque_sides * VBO_WIDTH * sizeof(int));
    int* liquid_data = malloc(total_liquid_sides * VBO_WIDTH * sizeof(int));

    // Check for allocation failure
    if (!transparent_data || !opaque_data || !liquid_data) {
        free(transparent_data);
        free(opaque_data);
        free(liquid_data);
        assert(false && "Failed to allocate memory for world mesh data\n");
    }

    // Second pass: Copy data
    int transparent_offset = 0;
    int opaque_offset = 0;
    int liquid_offset = 0;
    for (int i = 0; i < count; i++) {
        chunk_mesh* mesh = packet[i];

        chunk_mesh_to_buffer(transparent_data + transparent_offset, 
            mesh->transparent_sides, 
            mesh->num_transparent_sides);
        transparent_offset += mesh->num_transparent_sides * VBO_WIDTH;
        
        chunk_mesh_to_buffer(opaque_data + opaque_offset, 
            mesh->opaque_sides, 
            mesh->num_opaque_sides);
        opaque_offset += mesh->num_opaque_sides * VBO_WIDTH;
        
        chunk_mesh_to_buffer(liquid_data + liquid_offset, 
            mesh->liquid_sides, 
            mesh->num_liquid_sides);
        liquid_offset += mesh->num_liquid_sides * VBO_WIDTH;
    }

    // Create the world mesh structure
    world_mesh* world = malloc(sizeof(world_mesh));
    if (!world) {
        free(transparent_data);
        free(opaque_data);
        free(liquid_data);
        assert(false && "Failed to allocate memory for world mesh structure\n");
    }

    // Assign the data and counts to the world mesh
    world->transparent_data = transparent_data;
    world->opaque_data = opaque_data;
    world->liquid_data = liquid_data;
    world->num_transparent_sides = transparent_offset / VBO_WIDTH;
    world->num_opaque_sides = opaque_offset / VBO_WIDTH;
    world->num_liquid_sides = liquid_offset / VBO_WIDTH;

    return world;
}

void free_world_mesh(world_mesh* mesh) {
    if (mesh == NULL) {
        return;
    }
    free(mesh->transparent_data);
    free(mesh->opaque_data);
    free(mesh->liquid_data);
    free(mesh);
}

void get_world_mesh(mesh_args* args) {
    int x = args->x;
    int z = args->z;

    lock_world_mesh();
    lock_chunk_mesh();
    world_mesh* world = create_world_mesh(args->packet, *args->num_packets);
    if (!world) {
        assert(false && "Failed to create world mesh\n");
    }

    // Free the original chunk meshes
    if (args->world_mesh != NULL) {
        free_world_mesh(args->world_mesh);
    }
    args->world_mesh = world; // Update the args to point to the new world mesh
    unlock_chunk_mesh();
    unlock_world_mesh();
}

void lock_world_mesh() {
    pthread_mutex_lock(&wm_mutex);
}

void unlock_world_mesh() {
    pthread_mutex_unlock(&wm_mutex);
}

void update_world_mesh(mesh_args* args) {
    if (args == NULL) {
        assert(false && "mesh_args pointer is NULL\n");
    }

    if (args->packet == NULL) {
        return;
    }

    get_world_mesh(args);

    usleep(TICK_RATE);
    update_world_mesh(args);
}

void start_world_mesh_updater(mesh_args* args) {
    pthread_t updater_thread;
    pthread_create(&updater_thread, NULL, (void* (*)(void*))update_world_mesh, args);
    pthread_detach(updater_thread);
}