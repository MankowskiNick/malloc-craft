#include "ambient_occlusion.h"

#include <stdbool.h>

#include "../../world/core/block.h"

static bool is_ao_solid(int x, int y, int z, chunk* c, chunk* adj_chunks[4]) {
    if (y < 0 || y >= CHUNK_HEIGHT) {
        return false;
    }

    // Handle cross-chunk boundaries
    chunk* target_chunk = c;
    int local_x = x;
    int local_z = z;

    if (x < 0) {
        target_chunk = adj_chunks[3]; // EAST (-X)
        local_x = CHUNK_SIZE + x;
    } else if (x >= CHUNK_SIZE) {
        target_chunk = adj_chunks[1]; // WEST (+X)
        local_x = x - CHUNK_SIZE;
    }

    if (z < 0) {
        target_chunk = adj_chunks[0]; // NORTH (-Z)
        local_z = CHUNK_SIZE + z;
    } else if (z >= CHUNK_SIZE) {
        target_chunk = adj_chunks[2]; // SOUTH (+Z)
        local_z = z - CHUNK_SIZE;
    }

    if (target_chunk == NULL) {
        return false;
    }

    if (local_x < 0
            || local_x >= CHUNK_SIZE
            || local_z < 0
            || local_z >= CHUNK_SIZE) {
        return false;
    }

    short block_id = 0;
    get_block_info(target_chunk->blocks[local_x][y][local_z], &block_id, NULL, NULL, NULL);

    if (block_id == get_block_id("air")) {
        return false;
    }

    block_type block = get_block_type(block_id);

    // Model blocks don't contribute to AO
    if (block.id == -1
            || block.is_custom_model
            || block.transparent
            || block.liquid) {
        return false;
    }

    return true;
}

// Calculate vertex AO value (0-3) based on edge and corner neighbors
static int vertex_ao(int side1, int side2, int corner) {
    if (side1 && side2) {
        return 0;  // Fully occluded
    }
    return 3 - (side1 + side2 + corner);
}

// Calculate AO for all 4 vertices of a face
// Returns packed int with 4 AO values (2 bits each): v0 | (v1 << 2) | (v2 << 4) | (v3 << 6)
int calculate_face_ao(int x, int y, int z, int face, chunk* c, chunk* adj_chunks[4]) {
    int ao[4];

    // Neighbor offsets for each face's 4 vertices
    // For each vertex, we need side1, side2, and corner neighbors
    // The vertex order matches the quad rendering order in the shader

    switch (face) {
        case 4: // UP (+Y)
            // Vertex 0 (0,0), Vertex 1 (1,0), Vertex 2 (1,1), Vertex 3 (0,1)
            ao[0] = vertex_ao(
                is_ao_solid(x-1, y+1, z, c, adj_chunks),
                is_ao_solid(x, y+1, z-1, c, adj_chunks),
                is_ao_solid(x-1, y+1, z-1, c, adj_chunks));
            ao[1] = vertex_ao(
                is_ao_solid(x+1, y+1, z, c, adj_chunks),
                is_ao_solid(x, y+1, z-1, c, adj_chunks),
                is_ao_solid(x+1, y+1, z-1, c, adj_chunks));
            ao[2] = vertex_ao(
                is_ao_solid(x+1, y+1, z, c, adj_chunks),
                is_ao_solid(x, y+1, z+1, c, adj_chunks),
                is_ao_solid(x+1, y+1, z+1, c, adj_chunks));
            ao[3] = vertex_ao(
                is_ao_solid(x-1, y+1, z, c, adj_chunks),
                is_ao_solid(x, y+1, z+1, c, adj_chunks),
                is_ao_solid(x-1, y+1, z+1, c, adj_chunks));
            break;

        case 5: // DOWN (-Y)
            ao[0] = vertex_ao(
                is_ao_solid(x-1, y-1, z, c, adj_chunks),
                is_ao_solid(x, y-1, z-1, c, adj_chunks),
                is_ao_solid(x-1, y-1, z-1, c, adj_chunks));
            ao[1] = vertex_ao(
                is_ao_solid(x+1, y-1, z, c, adj_chunks),
                is_ao_solid(x, y-1, z-1, c, adj_chunks),
                is_ao_solid(x+1, y-1, z-1, c, adj_chunks));
            ao[2] = vertex_ao(
                is_ao_solid(x+1, y-1, z, c, adj_chunks),
                is_ao_solid(x, y-1, z+1, c, adj_chunks),
                is_ao_solid(x+1, y-1, z+1, c, adj_chunks));
            ao[3] = vertex_ao(
                is_ao_solid(x-1, y-1, z, c, adj_chunks),
                is_ao_solid(x, y-1, z+1, c, adj_chunks),
                is_ao_solid(x-1, y-1, z+1, c, adj_chunks));
            break;

        case 0: // NORTH (-Z)
            ao[0] = vertex_ao(
                is_ao_solid(x-1, y, z-1, c, adj_chunks),
                is_ao_solid(x, y-1, z-1, c, adj_chunks),
                is_ao_solid(x-1, y-1, z-1, c, adj_chunks));
            ao[1] = vertex_ao(
                is_ao_solid(x+1, y, z-1, c, adj_chunks),
                is_ao_solid(x, y-1, z-1, c, adj_chunks),
                is_ao_solid(x+1, y-1, z-1, c, adj_chunks));
            ao[2] = vertex_ao(
                is_ao_solid(x+1, y, z-1, c, adj_chunks),
                is_ao_solid(x, y+1, z-1, c, adj_chunks),
                is_ao_solid(x+1, y+1, z-1, c, adj_chunks));
            ao[3] = vertex_ao(
                is_ao_solid(x-1, y, z-1, c, adj_chunks),
                is_ao_solid(x, y+1, z-1, c, adj_chunks),
                is_ao_solid(x-1, y+1, z-1, c, adj_chunks));
            break;

        case 2: // SOUTH (+Z)
            ao[0] = vertex_ao(
                is_ao_solid(x-1, y, z+1, c, adj_chunks),
                is_ao_solid(x, y-1, z+1, c, adj_chunks),
                is_ao_solid(x-1, y-1, z+1, c, adj_chunks));
            ao[1] = vertex_ao(
                is_ao_solid(x+1, y, z+1, c, adj_chunks),
                is_ao_solid(x, y-1, z+1, c, adj_chunks),
                is_ao_solid(x+1, y-1, z+1, c, adj_chunks));
            ao[2] = vertex_ao(
                is_ao_solid(x+1, y, z+1, c, adj_chunks),
                is_ao_solid(x, y+1, z+1, c, adj_chunks),
                is_ao_solid(x+1, y+1, z+1, c, adj_chunks));
            ao[3] = vertex_ao(
                is_ao_solid(x-1, y, z+1, c, adj_chunks),
                is_ao_solid(x, y+1, z+1, c, adj_chunks),
                is_ao_solid(x-1, y+1, z+1, c, adj_chunks));
            break;

        case 1: // WEST (+X)
            ao[0] = vertex_ao(
                is_ao_solid(x+1, y, z-1, c, adj_chunks),
                is_ao_solid(x+1, y-1, z, c, adj_chunks),
                is_ao_solid(x+1, y-1, z-1, c, adj_chunks));
            ao[1] = vertex_ao(
                is_ao_solid(x+1, y, z+1, c, adj_chunks),
                is_ao_solid(x+1, y-1, z, c, adj_chunks),
                is_ao_solid(x+1, y-1, z+1, c, adj_chunks));
            ao[2] = vertex_ao(
                is_ao_solid(x+1, y, z+1, c, adj_chunks),
                is_ao_solid(x+1, y+1, z, c, adj_chunks),
                is_ao_solid(x+1, y+1, z+1, c, adj_chunks));
            ao[3] = vertex_ao(
                is_ao_solid(x+1, y, z-1, c, adj_chunks),
                is_ao_solid(x+1, y+1, z, c, adj_chunks),
                is_ao_solid(x+1, y+1, z-1, c, adj_chunks));
            break;

        case 3: // EAST (-X)
            ao[0] = vertex_ao(
                is_ao_solid(x-1, y, z-1, c, adj_chunks),
                is_ao_solid(x-1, y-1, z, c, adj_chunks),
                is_ao_solid(x-1, y-1, z-1, c, adj_chunks));
            ao[1] = vertex_ao(
                is_ao_solid(x-1, y, z+1, c, adj_chunks),
                is_ao_solid(x-1, y-1, z, c, adj_chunks),
                is_ao_solid(x-1, y-1, z+1, c, adj_chunks));
            ao[2] = vertex_ao(
                is_ao_solid(x-1, y, z+1, c, adj_chunks),
                is_ao_solid(x-1, y+1, z, c, adj_chunks),
                is_ao_solid(x-1, y+1, z+1, c, adj_chunks));
            ao[3] = vertex_ao(
                is_ao_solid(x-1, y, z-1, c, adj_chunks),
                is_ao_solid(x-1, y+1, z, c, adj_chunks),
                is_ao_solid(x-1, y+1, z-1, c, adj_chunks));
            break;

        default:
            ao[0] = ao[1] = ao[2] = ao[3] = 3;  // No occlusion
            break;
    }

    // Pack 4 AO values into single int (2 bits each)
    return ao[0] | (ao[1] << 2) | (ao[2] << 4) | (ao[3] << 6);
}
