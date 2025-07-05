#include <world_mesh.h>
#include <settings.h>
#include <assert.h>
#include <string.h>
#include <sort.h>
#include <util.h>
#include <mesh.h>

camera* wm_cam_ref = NULL;

void wm_init(camera* camera) {
    wm_cam_ref = camera;
    chunk_mesh_init(camera);
}

float chunk_distance_to_camera(const void* item) {
    chunk_mesh* packet = *(chunk_mesh**)item;
    // camera coords to chunk coords
    float x = (wm_cam_ref->position[0] / (float)CHUNK_SIZE);
    float z = (wm_cam_ref->position[2] / (float)CHUNK_SIZE);

    return -1.0f * sqrt(
        pow((float)(packet->x + 0.5f) - x, 2) +
        pow((float)(packet->z + 0.5f) - z, 2)
    );
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

world_mesh* get_world_mesh(get_world_mesh_args* args) {

    if (wm_cam_ref == NULL) {
        assert(false && "World mesh not initialized. Call wm_init(camera) first.\n");
    }

    if (args == NULL) {
        assert(false && "get_world_mesh_args pointer is NULL\n");
    }

    int* num_packets = args->num_packets;
    if (num_packets == NULL) {
        assert(false && "num_packets pointer is NULL\n");
    }
    int x = args->x;
    int z = args->z;

    int player_chunk_x = CAMERA_POS_TO_CHUNK_POS(x);
    int player_chunk_z = CAMERA_POS_TO_CHUNK_POS(z);

    int movedBlocks = ((int)x == (int)(wm_cam_ref->position[0]) && (int)z == (int)(wm_cam_ref->position[2])) ? 0 : 1;
    if (movedBlocks) {
        wm_cam_ref->position[0] = x;
        wm_cam_ref->position[2] = z;
    }

    chunk_mesh** packet = NULL;
    int count = 0;

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = player_chunk_x - CHUNK_RENDER_DISTANCE + i;
            int z = player_chunk_z - CHUNK_RENDER_DISTANCE + j;

            if (sqrt(pow(x - player_chunk_x, 2) + pow(z - player_chunk_z, 2)) > CHUNK_RENDER_DISTANCE) {
                continue;
            }

            chunk_mesh* mesh = get_chunk_mesh(x, z);

            if (mesh == NULL) {
                continue;
            }

            packet = realloc(packet, (count + 1) * sizeof(chunk_mesh*));
            packet[count] = mesh;
            count++;

            if (x >= player_chunk_x - 1 
                && x <= player_chunk_x + 1 
                && z >= player_chunk_z - 1
                && z <= player_chunk_z + 1
                && movedBlocks) {
                queue_chunk_for_sorting(mesh);
            }
        }
    }

    sort_chunk();
    load_chunk();

    quicksort(packet, count, sizeof(chunk_mesh*), chunk_distance_to_camera);

    *num_packets = count;

    world_mesh* world = create_world_mesh(packet, count);
    if (!world) {
        assert(false && "Failed to create world mesh\n");
    }

    // Free the original chunk meshes
    free(packet);
    
    return world;
}
