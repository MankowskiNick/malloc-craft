#include <world_mesh.h>
#include <assert.h>
#include <string.h>


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
        
        memcpy(transparent_data + transparent_offset, 
            mesh->transparent_data, 
            mesh->num_transparent_sides * VBO_WIDTH * sizeof(int));
        transparent_offset += mesh->num_transparent_sides * VBO_WIDTH;
        
        memcpy(opaque_data + opaque_offset, 
            mesh->opaque_data, 
            mesh->num_opaque_sides * VBO_WIDTH * sizeof(int));
        opaque_offset += mesh->num_opaque_sides * VBO_WIDTH;
        
        memcpy(liquid_data + liquid_offset, 
            mesh->liquid_data, 
            mesh->num_liquid_sides * VBO_WIDTH * sizeof(int));
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