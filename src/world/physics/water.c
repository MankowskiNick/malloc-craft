#include "water.h"
#include <util.h>
#include <util/settings.h>
#include <mesh.h>
#include <world/core/chunk.h>
#include <world/core/world.h>
#include <world/core/block.h>
#include <chunk_mesh.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// Water simulation constants - Minecraft-style
#define WATER_SOURCE_LEVEL 6           // Source water (full block)
#define WATER_SOURCE_MIN 6             // Minimum level that counts as a source
#define WATER_LEVEL_DROPOFF 2          // How much water level decreases per block horizontally

// Performance limits
#define MAX_CHUNKS_PER_TICK 16
#define MAX_UPDATES_PER_TICK 1000

// Encode/decode chunk coordinates to/from a unique ID
#define ENCODE_CHUNK_ID(x, z) ((int)(((((x) + 32768) & 0xFFFF) << 16) | (((z) + 32768) & 0xFFFF)))
#define DECODE_CHUNK_ID_X(id) ((int)(((id) >> 16) & 0xFFFF) - 32768)
#define DECODE_CHUNK_ID_Z(id) ((int)((id) & 0xFFFF) - 32768)

// Horizontal direction offsets (matches NORTH=0, WEST=1, SOUTH=2, EAST=3)
static const int DIR_OFFSETS[4][2] = {
    {0, -1},   // NORTH (z-)
    {1, 0},    // WEST (x+)
    {0, 1},    // SOUTH (z+)
    {-1, 0}    // EAST (x-)
};

// Water update entry
typedef struct {
    int chunk_x, chunk_z;
    int x, y, z;
    short new_level;
    bool set_water_block;  // true = set to water block, false = just update level
} water_update_t;

// Static update queues
static water_update_t* pending_updates = NULL;
static int num_updates = 0;
static int updates_capacity = 0;

static int* modified_chunks = NULL;
static int num_modified = 0;
static int modified_capacity = 0;

// Cached block IDs
static short water_id = -1;
static short air_id = -1;
static short torch_id = -1;

static void init_block_ids(void) {
    if (water_id < 0) {
        water_id = get_block_id("water");
        air_id = get_block_id("air");
        torch_id = get_block_id("torch");
    }
}

static void queue_water_update(int chunk_x, int chunk_z, int x, int y, int z,
                                short new_level, bool set_water_block) {
    if (num_updates >= MAX_UPDATES_PER_TICK) return;

    if (num_updates >= updates_capacity) {
        int new_cap = updates_capacity == 0 ? 256 : updates_capacity * 2;
        water_update_t* new_buf = realloc(pending_updates, new_cap * sizeof(water_update_t));
        if (!new_buf) return;
        pending_updates = new_buf;
        updates_capacity = new_cap;
    }

    pending_updates[num_updates++] = (water_update_t){
        .chunk_x = chunk_x,
        .chunk_z = chunk_z,
        .x = x,
        .y = y,
        .z = z,
        .new_level = new_level,
        .set_water_block = set_water_block
    };
}

static void mark_chunk_modified(int chunk_x, int chunk_z) {
    int chunk_id = ENCODE_CHUNK_ID(chunk_x, chunk_z);

    // Check if already marked
    for (int i = 0; i < num_modified; i++) {
        if (modified_chunks[i] == chunk_id) return;
    }

    if (num_modified >= modified_capacity) {
        int new_cap = modified_capacity == 0 ? 32 : modified_capacity * 2;
        int* new_buf = realloc(modified_chunks, new_cap * sizeof(int));
        if (!new_buf) return;
        modified_chunks = new_buf;
        modified_capacity = new_cap;
    }

    modified_chunks[num_modified++] = chunk_id;
}

// Check if a block can be destroyed by water
static bool is_water_destroyable(short block_id) {
    if (block_id == air_id || block_id == water_id) return false;

    block_type* type = get_block_type(block_id);
    if (!type) return false;

    // Foliage blocks are destroyed by water
    if (type->is_foliage) return true;

    // Torches are destroyed by water
    if (block_id == torch_id) return true;

    return false;
}

// Check if water can flow into a block position
static bool can_water_flow_into(short block_id) {
    if (block_id == air_id) return true;
    if (block_id == water_id) return true;
    if (is_water_destroyable(block_id)) return true;
    return false;
}

// Get block info at a position, handling chunk boundaries
// Returns false if position is invalid
static bool get_block_at(chunk* c, chunk* adj[4], int x, int y, int z,
                         short* block_id, short* water_level,
                         chunk** out_chunk, int* out_x, int* out_z) {
    if (y < 0 || y >= CHUNK_HEIGHT) return false;

    chunk* target = c;
    int tx = x, tz = z;

    // Handle X boundary
    if (x < 0) {
        if (!adj[3]) return false;  // EAST neighbor (x-)
        target = adj[3];
        tx = CHUNK_SIZE - 1;
    } else if (x >= CHUNK_SIZE) {
        if (!adj[1]) return false;  // WEST neighbor (x+)
        target = adj[1];
        tx = 0;
    }

    // Handle Z boundary
    if (z < 0) {
        if (!adj[0]) return false;  // NORTH neighbor (z-)
        target = adj[0];
        tz = CHUNK_SIZE - 1;
    } else if (z >= CHUNK_SIZE) {
        if (!adj[2]) return false;  // SOUTH neighbor (z+)
        target = adj[2];
        tz = 0;
    }

    get_block_info(target->blocks[tx][y][tz], block_id, NULL, NULL, water_level);

    if (out_chunk) *out_chunk = target;
    if (out_x) *out_x = tx;
    if (out_z) *out_z = tz;

    return true;
}

// Calculate what water level a position should have based on neighbors
static short calculate_flow_level(chunk* c, chunk* adj[4], int x, int y, int z,
                                   bool* should_become_source) {
    *should_become_source = false;

    short block_id, water_level;
    if (!get_block_at(c, adj, x, y, z, &block_id, &water_level, NULL, NULL, NULL)) {
        return 0;
    }

    // Check block above - falling water creates full block
    short above_id, above_water;
    if (get_block_at(c, adj, x, y + 1, z, &above_id, &above_water, NULL, NULL, NULL)) {
        if (above_id == water_id && above_water > 0) {
            return WATER_SOURCE_LEVEL;
        }
    }

    // Find highest adjacent horizontal water level and count sources
    short max_adjacent = 0;
    int source_neighbors = 0;

    for (int dir = 0; dir < 4; dir++) {
        int nx = x + DIR_OFFSETS[dir][0];
        int nz = z + DIR_OFFSETS[dir][1];

        short neighbor_id, neighbor_water;
        if (!get_block_at(c, adj, nx, y, nz, &neighbor_id, &neighbor_water, NULL, NULL, NULL)) {
            continue;
        }

        if (neighbor_id == water_id && neighbor_water > 0) {
            if (neighbor_water >= WATER_SOURCE_MIN) {
                source_neighbors++;
            }
            if (neighbor_water > max_adjacent) {
                max_adjacent = neighbor_water;
            }
        }
    }

    // Infinite source rule: 2+ adjacent source blocks create a new source
    if (source_neighbors >= 2 && block_id == water_id) {
        *should_become_source = true;
        return WATER_SOURCE_LEVEL;
    }

    // Flow level decreases by WATER_LEVEL_DROPOFF per block
    if (max_adjacent > WATER_LEVEL_DROPOFF) {
        return max_adjacent - WATER_LEVEL_DROPOFF;
    }

    return 0;
}

// Process a single water block for flow
static void process_water_block(game_data* data, chunk* c, chunk* adj[4],
                                 int x, int y, int z) {
    short block_id, water_level;
    get_block_info(c->blocks[x][y][z], &block_id, NULL, NULL, &water_level);

    // Only process water blocks with water level > 0
    if (block_id != water_id || water_level == 0) {
        return;
    }

    // STEP 1: Check for infinite source creation
    bool should_become_source = false;
    short new_level = calculate_flow_level(c, adj, x, y, z, &should_become_source);

    if (should_become_source && water_level < WATER_SOURCE_MIN) {
        queue_water_update(c->x, c->z, x, y, z, WATER_SOURCE_LEVEL, false);
        mark_chunk_modified(c->x, c->z);
    }

    // STEP 2: Flow downward (priority)
    bool can_flow_down = false;
    if (y > 0) {
        short below_id, below_water;
        chunk* below_chunk;
        int bx, bz;

        if (get_block_at(c, adj, x, y - 1, z, &below_id, &below_water,
                         &below_chunk, &bx, &bz)) {
            if (can_water_flow_into(below_id)) {
                can_flow_down = true;
                // Water below should be source level (falling water fills completely)
                if (below_id != water_id || below_water < WATER_SOURCE_LEVEL) {
                    bool set_block = (below_id != water_id);
                    queue_water_update(below_chunk->x, below_chunk->z, bx, y - 1, bz,
                                      WATER_SOURCE_LEVEL, set_block);
                    mark_chunk_modified(below_chunk->x, below_chunk->z);
                }
            }
        }
    }

    // STEP 3: Flow horizontally ONLY if we can't flow down
    // Water prioritizes going down - only spreads sideways when blocked below
    if (water_level > WATER_LEVEL_DROPOFF && !can_flow_down) {
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DIR_OFFSETS[dir][0];
            int nz = z + DIR_OFFSETS[dir][1];

            short neighbor_id, neighbor_water;
            chunk* neighbor_chunk;
            int ncx, ncz;

            if (!get_block_at(c, adj, nx, y, nz, &neighbor_id, &neighbor_water,
                              &neighbor_chunk, &ncx, &ncz)) {
                continue;
            }

            if (can_water_flow_into(neighbor_id)) {
                // Horizontal spread always decreases by WATER_LEVEL_DROPOFF
                short spread_level = water_level - WATER_LEVEL_DROPOFF;
                
                // Only update if we're providing higher water level
                if (spread_level > neighbor_water) {
                    bool set_block = (neighbor_id != water_id);
                    queue_water_update(neighbor_chunk->x, neighbor_chunk->z,
                                      ncx, y, ncz, spread_level, set_block);
                    mark_chunk_modified(neighbor_chunk->x, neighbor_chunk->z);
                }
            }
        }
    }
}

// Process water recalculation - handles water removal when source is gone
static void recalculate_water_block(game_data* data, chunk* c, chunk* adj[4],
                                     int x, int y, int z) {
    short block_id, water_level;
    get_block_info(c->blocks[x][y][z], &block_id, NULL, NULL, &water_level);

    if (block_id != water_id) return;

    bool should_become_source;
    short new_level = calculate_flow_level(c, adj, x, y, z, &should_become_source);

    if (should_become_source) {
        new_level = WATER_SOURCE_LEVEL;
    }

    // Water level should decrease if no longer supported
    if (new_level < water_level && water_level < WATER_SOURCE_MIN) {
        if (new_level == 0) {
            // Queue removal - will be replaced with air
            queue_water_update(c->x, c->z, x, y, z, 0, false);
        } else {
            queue_water_update(c->x, c->z, x, y, z, new_level, false);
        }
        mark_chunk_modified(c->x, c->z);
    }
}

// Apply all queued water updates
static void apply_water_updates(game_data* data) {
    for (int i = 0; i < num_updates; i++) {
        water_update_t* update = &pending_updates[i];

        chunk* c = get_chunk(update->chunk_x, update->chunk_z);
        if (!c) continue;

        short current_id, orientation, rot, current_water;
        get_block_info(c->blocks[update->x][update->y][update->z],
                       &current_id, &orientation, &rot, &current_water);

        if (update->new_level == 0 && current_id == water_id) {
            // Remove water block
            set_block_info(data, c, update->x, update->y, update->z,
                          air_id, (short)UNKNOWN_SIDE, 0, 0);
        } else if (update->set_water_block || current_id == water_id) {
            // Set or update water block
            set_block_info(data, c, update->x, update->y, update->z,
                          water_id, (short)DOWN, 0, update->new_level);
        }
    }
}

// Update meshes for all modified chunks
static void update_modified_meshes(game_data* data) {
    if (num_modified == 0) return;

    // Safety check - don't update meshes if world_mesh not initialized
    if (data->world_mesh == NULL) return;

    lock_mesh();
    for (int i = 0; i < num_modified; i++) {
        int chunk_x = DECODE_CHUNK_ID_X(modified_chunks[i]);
        int chunk_z = DECODE_CHUNK_ID_Z(modified_chunks[i]);
        update_chunk_mesh(chunk_x, chunk_z, data->player->position[0], data->player->position[2]);
    }
    // Signal that world mesh needs rebuilding
    data->mesh_requires_update = true;
    unlock_mesh();
}

/**
 * Main water flooding function
 * Processes all chunks marked for water flow
 */
void flood_chunks(game_data* data) {
    if (data == NULL || data->num_chunks_to_flow == 0) {
        return;
    }

    init_block_ids();

    // Reset update queues
    num_updates = 0;
    num_modified = 0;

    // Copy chunks list (processing may add new chunks)
    int num_chunks = data->num_chunks_to_flow;
    if (num_chunks > MAX_CHUNKS_PER_TICK) {
        num_chunks = MAX_CHUNKS_PER_TICK;
    }

    int* chunk_ids = malloc(num_chunks * sizeof(int));
    if (!chunk_ids) return;
    memcpy(chunk_ids, data->chunks_to_flow, num_chunks * sizeof(int));

    // Remove processed chunks from the list
    if (num_chunks < data->num_chunks_to_flow) {
        // Shift remaining chunks
        memmove(data->chunks_to_flow,
                data->chunks_to_flow + num_chunks,
                (data->num_chunks_to_flow - num_chunks) * sizeof(int));
        data->num_chunks_to_flow -= num_chunks;
    } else {
        data->num_chunks_to_flow = 0;
    }

    // Process each chunk
    for (int i = 0; i < num_chunks; i++) {
        int chunk_x = DECODE_CHUNK_ID_X(chunk_ids[i]);
        int chunk_z = DECODE_CHUNK_ID_Z(chunk_ids[i]);

        chunk* c = get_chunk(chunk_x, chunk_z);
        if (!c) continue;

        // Get adjacent chunks
        chunk* adj[4] = {
            get_chunk(chunk_x, chunk_z - 1),  // NORTH
            get_chunk(chunk_x + 1, chunk_z),  // WEST
            get_chunk(chunk_x, chunk_z + 1),  // SOUTH
            get_chunk(chunk_x - 1, chunk_z)   // EAST
        };

        // Process from top to bottom for proper gravity handling
        for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    process_water_block(data, c, adj, x, y, z);
                    recalculate_water_block(data, c, adj, x, y, z);
                }
            }
        }
    }

    // Apply all updates
    apply_water_updates(data);

    // Update meshes
    update_modified_meshes(data);

    free(chunk_ids);
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

