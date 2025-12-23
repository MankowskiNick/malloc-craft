#include <water.h>
#include <util.h>
#include <settings.h>
#include <mesh.h>
#include <chunk.h>
#include <world.h>
#include <block.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Water simulation constants - Minecraft-style
// We use: level 7/6 = source, levels 5-1 = flowing (7 is strongest, 1 is weakest)
#define WATER_MAX_FLOW_LEVEL 5         // Maximum flowing level (strongest flow)
#define WATER_MIN_FLOW_LEVEL 1         // Minimum flowing level (weakest flow)

// Encode/decode chunk coordinates to/from a unique ID
#define ENCODE_CHUNK_ID(x, z) ((int)(((((x) + 32768) & 0xFFFF) << 16) | (((z) + 32768) & 0xFFFF)))
#define DECODE_CHUNK_ID_X(id) ((int)(((id) >> 16) & 0xFFFF) - 32768)
#define DECODE_CHUNK_ID_Z(id) ((int)((id) & 0xFFFF) - 32768)

/**
 * Main water flooding function
 * Processes all chunks marked for water flow
 */
void flood_chunks(game_data* data) {

}

/**
 * Called when a block is placed or modified
 * Marks affected chunks for water flow simulation
 */
void check_for_flow(game_data* data, chunk* c, chunk* adj[4], int x, int y, int z) {
    if (data == NULL || c == NULL) {
        return;
    }

    short water_id = get_block_id("water");
    bool should_flow = false;

    // Check if this block is water or adjacent to water
    short block_id = 0;
    get_block_info(c->blocks[x][y][z], &block_id, NULL, NULL, NULL);

    if (block_id == water_id) {
        should_flow = true;
    } else {
        // Check all 6 neighbors for water
        for (int side = 0; side < 6; side++) {
            chunk* neighbor_chunk = NULL;
            if (side < 4) {
                neighbor_chunk = adj[side];
            }

            short adjacent_id = get_adjacent_block_id(x, y, z, side, c, neighbor_chunk);
            if (adjacent_id == water_id) {
                should_flow = true;
                break;
            }
        }
    }

    if (!should_flow) {
        return;
    }

    // Mark this chunk for water flow
    int chunk_id = ENCODE_CHUNK_ID(c->x, c->z);
    bool already_marked = false;
    for (int i = 0; i < data->num_chunks_to_flow; i++) {
        if (data->chunks_to_flow[i] == chunk_id) {
            already_marked = true;
            break;
        }
    }

    if (!already_marked) {
        int* temp = realloc(data->chunks_to_flow, (data->num_chunks_to_flow + 1) * sizeof(int));
        if (temp == NULL) {
            return;
        }
        data->chunks_to_flow = temp;
        data->chunks_to_flow[data->num_chunks_to_flow++] = chunk_id;
    }

    // Also mark adjacent chunks if block is at boundary
    if (x == 0 && adj[3] != NULL) {
        int adj_id = ENCODE_CHUNK_ID(c->x - 1, c->z);
        bool found = false;
        for (int i = 0; i < data->num_chunks_to_flow; i++) {
            if (data->chunks_to_flow[i] == adj_id) { found = true; break; }
        }
        if (!found) {
            int* temp = realloc(data->chunks_to_flow, (data->num_chunks_to_flow + 1) * sizeof(int));
            if (temp != NULL) {
                data->chunks_to_flow = temp;
                data->chunks_to_flow[data->num_chunks_to_flow++] = adj_id;
            }
        }
    }
    if (x == CHUNK_SIZE - 1 && adj[1] != NULL) {
        int adj_id = ENCODE_CHUNK_ID(c->x + 1, c->z);
        bool found = false;
        for (int i = 0; i < data->num_chunks_to_flow; i++) {
            if (data->chunks_to_flow[i] == adj_id) { found = true; break; }
        }
        if (!found) {
            int* temp = realloc(data->chunks_to_flow, (data->num_chunks_to_flow + 1) * sizeof(int));
            if (temp != NULL) {
                data->chunks_to_flow = temp;
                data->chunks_to_flow[data->num_chunks_to_flow++] = adj_id;
            }
        }
    }
    if (z == 0 && adj[0] != NULL) {
        int adj_id = ENCODE_CHUNK_ID(c->x, c->z - 1);
        bool found = false;
        for (int i = 0; i < data->num_chunks_to_flow; i++) {
            if (data->chunks_to_flow[i] == adj_id) { found = true; break; }
        }
        if (!found) {
            int* temp = realloc(data->chunks_to_flow, (data->num_chunks_to_flow + 1) * sizeof(int));
            if (temp != NULL) {
                data->chunks_to_flow = temp;
                data->chunks_to_flow[data->num_chunks_to_flow++] = adj_id;
            }
        }
    }
    if (z == CHUNK_SIZE - 1 && adj[2] != NULL) {
        int adj_id = ENCODE_CHUNK_ID(c->x, c->z + 1);
        bool found = false;
        for (int i = 0; i < data->num_chunks_to_flow; i++) {
            if (data->chunks_to_flow[i] == adj_id) { found = true; break; }
        }
        if (!found) {
            int* temp = realloc(data->chunks_to_flow, (data->num_chunks_to_flow + 1) * sizeof(int));
            if (temp != NULL) {
                data->chunks_to_flow = temp;
                data->chunks_to_flow[data->num_chunks_to_flow++] = adj_id;
            }
        }
    }
}
