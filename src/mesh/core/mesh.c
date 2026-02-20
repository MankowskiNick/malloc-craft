#include "mesh.h"

#include <pthread.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../util/settings.h"
#include "../../util/queue.h"
#include "../../world/core/block.h"
#include "../../world/core/world.h"
#include "../../player/core/camera.h"
#include "../generation/chunk_mesh.h"
#include "../geometry/blockbench_loader.h"
#include "worker_pool.h"

// Hashmap keyed by (chunk_coordinate + LOD level) for efficient multi-LOD caching
DEFINE_HASHMAP(chunk_mesh_lod_map, chunk_mesh_key, chunk_mesh, chunk_mesh_key_hash, chunk_mesh_key_equals);
typedef chunk_mesh_lod_map_hashmap chunk_mesh_lod_map;

// Active and staging buffers for double-buffering
chunk_mesh_lod_map chunk_packets;
chunk_mesh_lod_map chunk_packets_buffer;
pthread_mutex_t packet_swap_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t packet_update_signal = PTHREAD_COND_INITIALIZER;

// Worker pool for chunk mesh generation
worker_pool* chunk_worker_pool = NULL;

// Mutexes for shared structures
pthread_mutex_t chunk_packets_mutex;
pthread_mutex_t sort_queue_mutex;
pthread_mutex_t chunk_load_queue_mutex;

queue_node* sort_queue = NULL;
queue_node* chunk_load_queue = NULL;

void m_init(camera* camera) {
    chunk_packets = chunk_mesh_lod_map_init(CHUNK_CACHE_SIZE);
    chunk_packets_buffer = chunk_mesh_lod_map_init(CHUNK_CACHE_SIZE);  // Initialize staging buffer
    queue_init(&sort_queue);
    queue_init(&chunk_load_queue);
    chunk_mesh_init(camera);

    pthread_mutex_init(&chunk_packets_mutex, NULL);
    pthread_mutex_init(&sort_queue_mutex, NULL);
    pthread_mutex_init(&chunk_load_queue_mutex, NULL);
    
    // Initialize worker pool for chunk mesh generation
    chunk_worker_pool = pool_init(WORKER_THREADS, (work_processor_fn)process_chunk_work_item);
    if (chunk_worker_pool == NULL) {
        fprintf(stderr, "Failed to initialize worker pool with %d threads\n", WORKER_THREADS);
        exit(EXIT_FAILURE);
    }
}

void m_cleanup() {
    // Shutdown worker pool
    if (chunk_worker_pool != NULL) {
        pool_shutdown(chunk_worker_pool);
        chunk_worker_pool = NULL;
    }
    
    chunk_mesh_lod_map_free(&chunk_packets);
    chunk_mesh_lod_map_free(&chunk_packets_buffer);  // Free staging buffer
    queue_cleanup(&sort_queue);
    queue_cleanup(&chunk_load_queue);

    pthread_mutex_destroy(&chunk_packets_mutex);
    pthread_mutex_destroy(&sort_queue_mutex);
    pthread_mutex_destroy(&chunk_load_queue_mutex);
    pthread_mutex_destroy(&packet_swap_mutex);
    pthread_cond_destroy(&packet_update_signal);
}

void preload_initial_chunks(game_data* data) {
    if (chunk_worker_pool == NULL || data == NULL) {
        return;
    }

    float player_x = data->player->position[0];
    float player_z = data->player->position[2];

    // Calculate how many chunks we need to load
    int player_chunk_x = WORLD_POS_TO_CHUNK_POS(player_x);
    int player_chunk_z = WORLD_POS_TO_CHUNK_POS(player_z);

    int chunks_queued = 0;

    // Spiral outward from the player so nearest chunks are queued first.
    // Uses the standard square-spiral: right, down, left, up with increasing segment lengths.
    int sx = player_chunk_x;
    int sz = player_chunk_z;
    // dx/dz pairs: right (+x), down (+z), left (-x), up (-z)
    int dir_dx[] = {1, 0, -1, 0};
    int dir_dz[] = {0, 1, 0, -1};
    int dir = 0, seg_len = 1, seg_pos = 0, segs_done = 0;
    int max_side = 2 * TRUE_RENDER_DISTANCE + 1;
    int max_iter = max_side * max_side;

    for (int i = 0; i < max_iter; i++) {
        if (sqrt(pow(sx - player_chunk_x, 2) + pow(sz - player_chunk_z, 2)) <= TRUE_RENDER_DISTANCE) {
            chunk_mesh* existing = get_chunk_mesh(sx, sz);
            if (existing == NULL) {
                chunks_queued++;
            }
        }

        sx += dir_dx[dir];
        sz += dir_dz[dir];
        if (++seg_pos == seg_len) {
            seg_pos = 0;
            dir = (dir + 1) % 4;
            if (++segs_done % 2 == 0) {
                seg_len++;
            }
        }
    }

    // Submit all queued chunks to workers - they will be processed in the background
    if (chunks_queued > 0) {
        for (int i = 0; i < chunks_queued; i++) {
            load_chunk(player_x, player_z);
        }
    }
}

block_data_t get_block_data(int x, int y, int z, chunk* c) {
    block_data_t data = {
        .bytes = { 0, 0, 0 }
    };
    if (c == NULL) {
        printf("ERROR: Cannot get block data from NULL chunk.\n");
        return data;
    }
    
    // Validate bounds
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return data;  // Return air block (0) for out-of-bounds access
    }
    
    return c->blocks[x][y][z];
}

block_data_t get_adjacent_block_data(int x, int y, int z, short side, short lod_scale, chunk* c, chunk* adj) {
    switch(side) {
        case (int)UP:
            if (y + lod_scale < CHUNK_HEIGHT) {
                return c->blocks[x][y + lod_scale][z];
            }
            break;
        case (int)DOWN:
            if (y - lod_scale >= 0) {
                return c->blocks[x][y - lod_scale][z];
            }
            break;
        case (int)WEST:
            if (x + lod_scale < CHUNK_SIZE) {
                return c->blocks[x + lod_scale][y][z];
            }
            else if (adj != NULL) {
                // Calculate how far we've overshot into the adjacent chunk
                int overflow = (x + lod_scale) - CHUNK_SIZE;
                int adj_x = (overflow > 0) ? overflow : 0;
                if (adj_x < CHUNK_SIZE) {
                    return adj->blocks[adj_x][y][z];
                }
            }
            break;
        case (int)EAST:
            if (x - lod_scale >= 0) {
                return c->blocks[x - lod_scale][y][z];
            }
            else if (adj != NULL) {
                // Calculate position in adjacent chunk when underflowing
                int underflow = lod_scale - x;
                int adj_x = CHUNK_SIZE - underflow;
                if (adj_x >= 0 && adj_x < CHUNK_SIZE) {
                    return adj->blocks[adj_x][y][z];
                }
            }
            break;
        case (int)NORTH:
            if (z - lod_scale >= 0) {
                return c->blocks[x][y][z - lod_scale];
            }
            else if (adj != NULL) {
                // Calculate position in adjacent chunk when underflowing
                int underflow = lod_scale - z;
                int adj_z = CHUNK_SIZE - underflow;
                if (adj_z >= 0 && adj_z < CHUNK_SIZE) {
                    return adj->blocks[x][y][adj_z];
                }
            }
            break;
        case (int)SOUTH:
            if (z + lod_scale < CHUNK_SIZE) {
                return c->blocks[x][y][z + lod_scale];
            }
            else if (adj != NULL) {
                // Calculate how far we've overshot into the adjacent chunk
                int overflow = (z + lod_scale) - CHUNK_SIZE;
                int adj_z = (overflow > 0) ? overflow : 0;
                if (adj_z < CHUNK_SIZE) {
                    return adj->blocks[x][y][adj_z];
                }
            }
            break;
        default:
            break;
    }
    
    // Return air block (0) if we've reached this point
    // This happens when:
    // - UP/DOWN sides exceed chunk height boundaries
    // - Horizontal sides need adjacent chunk but adj is NULL or out of bounds after overflow
    block_data_t data = { .bytes = { 0, 0, 0 } };
    return data;
}

short get_adjacent_block_id(int x, int y, int z, short side, chunk* c, chunk* adj) {
    block_data_t block_data = get_adjacent_block_data(x, y, z, side, 1, c, adj); // use default LOD: 1
    short block_id = -1;
    get_block_info(block_data, &block_id, NULL, NULL, NULL);
    return block_id;
}

short get_adjacent_block(int x, int y, int z, short side, short lod_scale, chunk* c, chunk* adj) {
    block_data_t block_data = get_adjacent_block_data(x, y, z, side, lod_scale, c, adj);
    short block_id = 0;
    get_block_info(block_data, &block_id, NULL, NULL, NULL);
    return block_id;
}

// TODO: this function is messy, clean it up
void get_side_visible(
    int x, int y, int z,
    short side,
    short lod_scale,
    chunk* c,
    chunk* adj,
    int* visible_out,
    int* underwater_out,
    int* water_level_out
) {
    // calculate adjacent block
    short adjacent_id = get_adjacent_block(x, y, z, side, lod_scale, c, adj);

    short current_id = 0;

    short current_water_level = 0;
    get_block_info(c->blocks[x][y][z], &current_id, NULL, NULL, &current_water_level);
    block_type* current = get_block_type(current_id);
    if (current == NULL) {
        *visible_out = 0;
        return;  // Invalid block, don't render
    }

    // calculate visibility
    block_type* adjacent = get_block_type(adjacent_id);
    if (adjacent == NULL) {
        *visible_out = 0;
        return;  // Invalid adjacent block, don't render
    }
    uint visible = adjacent_id == get_block_id("air") || adjacent->transparent != current->transparent;

    // check if we are underwater
    if (adjacent_id == get_block_id("water")) {
        *underwater_out = 1;
    }

    // get water level from adjacent block
    short adj_water_level = 0;
    if (adjacent_id == get_block_id("water")) {
        block_data_t adj_block_data = get_adjacent_block_data(x, y, z, side, lod_scale, c, adj);
        get_block_info(adj_block_data, NULL, NULL, NULL, &adj_water_level);
        *water_level_out = (int)adj_water_level;
    } else {
        *water_level_out = 0;
    }

    // For liquid blocks: show face at any boundary where we can see the water volume
    // This includes: water-to-air, water-to-solid, but NOT water-to-water
    if (current->liquid) {
        // Hide face if adjacent is also liquid (same type)
        if (adjacent != NULL && adjacent->liquid) {
            visible = 0;
        }
        // Show face if adjacent is not water (air, solid blocks, etc.)
        else if (adjacent_id != get_block_id("water")) {
            visible = 1;
        }
    }

    // make sure transparent neighbors are visible
    if (adjacent != NULL && adjacent->transparent != current->transparent) {
        visible = 1;
    }

    if (adjacent->is_custom_model) {
        visible = 1;
    }

    if (adjacent != NULL
        && (adjacent->transparent && current->transparent)
        && adjacent->id != current->id) {
        visible = 1;
    }

    if (current->is_foliage) {
        visible = 1; // foliage is always visible
    }

    // dont render sides that we can't see
    switch(side) {
        case (int)UP:
            visible = visible && y < CHUNK_HEIGHT;
            break;
        case (int)DOWN:
            visible = visible && y > 0;
            break;
        default:
            break;
    }

    *visible_out = visible;
}

short get_rotated_side(int side, short rot) {
    if (rot == 0) {
        return side;
    }

    // only rotate sides 0-3 (horizontal sides) (4, 5, 6 are up, down, unknown)
    if (side > 3) {
        return side;
    }

    int new_side = (side + rot) % 4;
    return new_side;
}

short get_converted_side(int side, short orientation) {
    if (orientation == (short)UNKNOWN_SIDE) {
        return side;
    }
    
    // rotate side based on orientation
    int new_side = side;

    switch(orientation) {

        // down is default

        case (short)UP:
            if (side == (short)UP) {
                new_side = (short)DOWN;
            }
            else if (side == (short)DOWN) {
                new_side = (short)UP;
            }
            else if (side == (short)NORTH) {
                new_side = (short)SOUTH;
            }
            else if (side == (short)SOUTH) {
                new_side = (short)NORTH;
            }
            break;


        case (short)NORTH:
            if (side == (short)NORTH) {
                new_side = (short)UP;
            }
            else if (side == (short)SOUTH) {
                new_side = (short)DOWN;
            }
            else if (side == (short)UP) {
                new_side = (short)SOUTH;
            }
            else if (side == (short)DOWN) {
                new_side = (short)NORTH;
            }
            break;

        case (short)SOUTH:
            if (side == (short)NORTH) {
                new_side = (short)UP;
            }
            else if (side == (short)SOUTH) {
                new_side = (short)DOWN;
            }
            else if (side == (short)UP) {
                new_side = (short)SOUTH;
            }
            else if (side == (short)DOWN) {
                new_side = (short)NORTH;
            }
            break;

        case (short)EAST:
            if (side == (short)EAST) {
                new_side = (short)DOWN;
            }
            else if (side == (short)WEST) {
                new_side = (short)UP;
            }
            else if (side == (short)UP) {
                new_side = (short)EAST;
            }
            else if (side == (short)DOWN) {
                new_side = (short)WEST;
            }
            break;

        case (short)WEST:
            if (side == (short)EAST) {
                new_side = (short)UP;
            }
            else if (side == (short)WEST) {
                new_side = (short)DOWN;
            }
            else if (side == (short)UP) {
                new_side = (short)WEST;
            }
            else if (side == (short)DOWN) {
                new_side = (short)EAST;
            }
            break;

        default:
            break;
    }

    return new_side;
}

void get_model_transformation(mat4 transform, block_type* block, short orientation, short rot) {
    // get orientation and rotation info
    glm_mat4_identity(transform);
    glm_translate(transform, (vec3){0.5f, 0.5f, 0.5f}); // translate to center of block for rotation
    if (block->oriented) {
        switch(orientation) {
            case (short)UP:
                glm_rotate(transform, (float)(M_PI), (vec3){1.0f, 0.0f, 0.0f});
                break;
            case (short)NORTH:
                // glm_rotate(transform, (float)(-M_PI / 2.0), (vec3){1.0f, 0.0f, 0.0f});
                break;
            case (short)SOUTH:
                glm_rotate(transform, (float)(M_PI), (vec3){0.0f, 1.0f, 0.0f});
                break;
            case (short)EAST:
                glm_rotate(transform, (float)(M_PI / 2.0), (vec3){0.0f, 1.0f, 0.0f});
                break;
            case (short)WEST:
                glm_rotate(transform, (float)(-M_PI / 2.0), (vec3){0.0f, 1.0f, 0.0f});
                break;
            default:
                break;
        }
    }
    else {
        glm_rotate(transform, (float)(rot * (M_PI / 2.0)), (vec3){0.0f, 1.0f, 0.0f});
    }
    glm_translate(transform, (vec3){-0.5f, -0.5f, -0.5f}); // translate back
}

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

    block_type* block = get_block_type(block_id);

    // Model blocks don't contribute to AO
    if (block == NULL 
            || block->is_custom_model
            || block->transparent
            || block->liquid) {
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

void pack_side(int x_0, int y_0, int z_0,
        short side,
        short orientation,
        short rot,
        short type,
        short water_level,
        bool underwater,
        int ao,
        side_instance* data) {
    data->x = x_0;
    data->y = y_0;
    data->z = z_0;
    data->side = side;
    data->water_level = water_level;
    data->water_level_transition = 0;  // Initialize transition field
    data->underwater = underwater ? 1 : 0;
    data->ao = ao;

    // block specific data
    block_type* block = get_block_type(type);
    if (block == NULL) {
        return;  // Invalid block type, skip packing
    }
    data->orientation = block->oriented ? orientation : (short)DOWN;

    short display_side = get_rotated_side(side, rot);
    if (!block->is_custom_model && !block->is_foliage && block->oriented) {
        display_side = get_converted_side(side, orientation);
    }
    // Foliage only uses sides 0 and 1, clamp to valid range
    if (block->is_foliage && display_side > 1) {
        display_side = side % 2;
    }
    data->atlas_x = block->face_atlas_coords[display_side][0];
    data->atlas_y = block->face_atlas_coords[display_side][1];
}

// Generate water flow transition faces between blocks with different water levels
void pack_water_transitions(
    int x, int y, int z,
    short lod_scale,
    chunk* c,
    chunk* adj_chunks[4],
    short current_water_level,
    side_instance** chunk_side_data,
    int* num_sides) {
    
    int world_x = x + (CHUNK_SIZE * c->x);
    int world_y = y;
    int world_z = z + (CHUNK_SIZE * c->z);

    short water_id = get_block_id("water");
    
    // Check 4 cardinal directions (0=NORTH, 1=WEST, 2=SOUTH, 3=EAST)
    for (int side = 0; side < 4; side++) {
        chunk* adj = adj_chunks[side];
        
        // Get adjacent block's water level
        short adj_block_id = get_adjacent_block(x, y, z, side, lod_scale, c, adj);
        if (adj_block_id != water_id) {
            continue;  // Only care about water-to-water transitions
        }
        
        block_data_t adj_block_data = get_adjacent_block_data(x, y, z, side, lod_scale, c, adj);
        short adj_water_level = 0;
        get_block_info(adj_block_data, NULL, NULL, NULL, &adj_water_level);
        
        // Generate transition if there's a height difference
        // Only generate from the higher level to avoid duplicates
        if (current_water_level == adj_water_level) {
            continue;  // Same level, no transition needed
        }
        
        // Determine which way the transition should face
        int should_transition = 0;
        short from_level = 0, to_level = 0;
        
        if (current_water_level > adj_water_level) {
            // Current is higher - transition faces away from current block
            should_transition = 1;
            from_level = adj_water_level;
            to_level = current_water_level;
        } else if (current_water_level < adj_water_level && adj_water_level - current_water_level == 1) {
            // Adjacent is only 1 level higher - add a transition from this side too
            // This creates the surface from the lower water level's perspective
            should_transition = 1;
            from_level = current_water_level;
            to_level = adj_water_level;
        }
        
        if (!should_transition) {
            continue;
        }
        
        // Allocate space for transition face
        int new_side_count = (*num_sides) + 1;
        if (new_side_count > SIDES_PER_CHUNK) {
            side_instance* tmp = realloc(*chunk_side_data, new_side_count * sizeof(side_instance));
            assert(tmp != NULL && "Failed to allocate memory for transition side data");
            *chunk_side_data = tmp;
        }
        
        // Create transition face
        side_instance* trans = &((*chunk_side_data)[*num_sides]);
        trans->x = world_x;
        trans->y = world_y;
        trans->z = world_z;
        trans->side = side;  // Reuse cardinal side type (0-3)
        trans->water_level = to_level;  // Higher level (target)
        trans->water_level_transition = from_level;  // Lower level (source)
        trans->underwater = 0;
        trans->orientation = (short)DOWN;
        trans->ao = 0xFF;  // No AO for water transitions (all vertices = 3)

        // Use water texture for transition
        block_type* water_block = get_block_type(water_id);
        if (water_block == NULL) {
            continue;  // Invalid water block type
        }
        short display_side = side;  // Use the cardinal direction directly
        trans->atlas_x = water_block->face_atlas_coords[display_side][0];
        trans->atlas_y = water_block->face_atlas_coords[display_side][1];

        (*num_sides)++;
    }
}

void pack_block(
    int x, int y, int z,
    short lod_scale,
    chunk* c,
    chunk* adj_chunks[4], // front, back, left, right
    side_instance** chunk_side_data,
    int* num_sides) {

    int world_x = x + (CHUNK_SIZE * c->x);
    int world_y = y;
    int world_z = z + (CHUNK_SIZE * c->z);

    short block_id = 0;
    short orientation = 0;
    short rot = 0;
    short current_water_level = 0;
    get_block_info(c->blocks[x][y][z], &block_id, &orientation, &rot, &current_water_level);

    for (int side = 0; side < 6; side++) {
        chunk* adj = NULL;
        if (side < 4) {
            adj = adj_chunks[side];
        }

        int visible = 0;
        int underwater = 0;
        int adj_water_level = 0;
        get_side_visible(x, y, z, side, lod_scale, c, adj, &visible, &underwater, &adj_water_level);
        if (!visible) {
            continue;
        }

        int new_side_count = (*num_sides) + 1;

        // check if we need to reallocate memory
        if (new_side_count > SIDES_PER_CHUNK) {
            side_instance* tmp = realloc(*chunk_side_data, new_side_count * sizeof(side_instance));
            assert(tmp != NULL && "Failed to allocate memory for side data");
            *chunk_side_data = tmp;
        }

        // For liquid blocks, use the current block's water level
        // For non-liquid blocks, use the adjacent water level (for underwater effects)
        block_type* block = get_block_type(block_id);
        if (block == NULL) {
            continue;  // Invalid block type, skip this face
        }
        short water_level_to_use = block->liquid ? current_water_level : (short)adj_water_level;

        // Calculate AO for this face
        int ao = calculate_face_ao(x, y, z, side, c, adj_chunks);

        pack_side(
            world_x, world_y, world_z,
            side,
            orientation, rot,
            block_id,
            water_level_to_use,
            underwater,
            ao,
            &((*chunk_side_data)[*num_sides])
        );
        (*num_sides)++;
    }
}

void pack_model(
    int x, int y, int z,
    chunk* c,
    float** custom_model_data,
    int* num_custom_verts
) {
    short block_id = 0;
    short orientation = 0;
    short rot = 0;
    short water_level = 0;
    get_block_info(c->blocks[x][y][z], &block_id, &orientation, &rot, &water_level);
    block_type* block = get_block_type(block_id);
    if (block == NULL || !block->is_custom_model || block->model == NULL) {
        return;
    }

    // reference blockbench model data hashmap based on model name
    blockbench_model* model = NULL;

    if (block->oriented) {
        model = get_blockbench_model(block->models[orientation]);
    }
    else {
        model = get_blockbench_model(block->model);
    }

    if (model == NULL) {
        return;
    }

    int new_vert_count = (*num_custom_verts) + model->index_count;
    if (new_vert_count > MODEL_VERTICES_PER_CHUNK) {
        float* tmp = realloc(*custom_model_data, new_vert_count * sizeof(float) * FLOATS_PER_MODEL_VERT);
        assert(tmp != NULL && "Failed to allocate memory for custom model data");
        *custom_model_data = tmp;
    }

    mat4 transformation;
    get_model_transformation(transformation, block, orientation, rot);

    // copy model data into chunk mesh data
    for (int i = 0; i < model->index_count; i++) {
        int dest_idx = (*num_custom_verts + i) * FLOATS_PER_MODEL_VERT;

        float dest_x = (float)x + (float)(c->x * CHUNK_SIZE);
        float dest_y = (float)y;
        float dest_z = (float)z + (float)(c->z * CHUNK_SIZE);
        blockbench_vertex vert = model->vertices[model->indices[i]];

        // apply transformation
        vec4 pos = {vert.position[0], vert.position[1], vert.position[2], 1.0f};
        vec4 normals = {vert.normal[0], vert.normal[1], vert.normal[2], 0.0f};
        
        vec4 transformed_vert, transformed_normals;
        glm_mat4_mulv(transformation, pos, transformed_vert);
        glm_mat4_mulv(transformation, normals, transformed_normals);

        // position
        (*custom_model_data)[dest_idx + 0] = dest_x + transformed_vert[0];
        (*custom_model_data)[dest_idx + 1] = dest_y + transformed_vert[1];
        (*custom_model_data)[dest_idx + 2] = dest_z + transformed_vert[2];

        // normal
        (*custom_model_data)[dest_idx + 3] = transformed_normals[0];
        (*custom_model_data)[dest_idx + 4] = transformed_normals[1];
        (*custom_model_data)[dest_idx + 5] = transformed_normals[2];

        // uv
        (*custom_model_data)[dest_idx + 6] = vert.uv[0];
        (*custom_model_data)[dest_idx + 7] = vert.uv[1];
    }

    *num_custom_verts = new_vert_count;
}

bool in_chunk_bounds(int x, int y, int z) {
    return x < CHUNK_SIZE && x >= 0 
        && y < CHUNK_HEIGHT && y >= 0 
        && z < CHUNK_SIZE && z >= 0 ;
}

void pack_chunk(chunk* c, chunk* adj_chunks[4],
    short lod_scale,
    side_instance** opaque_side_data, int* num_opaque_sides,
    side_instance** transparent_side_data, int* num_transparent_sides,
    side_instance** foliage_side_data, int* num_foliage_sides,
    side_instance** liquid_side_data, int* num_liquid_sides,
    float** custom_model_data, int* num_custom_verts,
    int render_transparent, int render_foliage) {
    if (c == NULL) {
        return;
    }

    for (int i = 0; i < CHUNK_SIZE; i+=lod_scale) {
        for (int j = 0; j < CHUNK_SIZE; j+=lod_scale) {
            for (int k = 0; k < CHUNK_HEIGHT; k+=lod_scale) {
                if (!in_chunk_bounds(i, k, j)) {
                    continue;
                }

                short block_id = 0;
                get_block_info(c->blocks[i][k][j], &block_id, NULL, NULL, NULL); // this may need to turn in to an average of a nxnxn box where n is lod_scale
                if (block_id == get_block_id("air")) {
                    continue;
                }

                block_type* block = get_block_type(block_id);
                if (block == NULL) {
                    continue;  // Invalid block type, skip
                }
                if (block->liquid) {
                    // Pack normal liquid faces
                    pack_block(i, k, j, lod_scale, c, adj_chunks,
                        liquid_side_data, num_liquid_sides);
                    
                    // Pack water flow transitions
                    short current_water_level = 0;
                    get_block_info(c->blocks[i][k][j], NULL, NULL, NULL, &current_water_level);
                    pack_water_transitions(i, k, j, lod_scale, c, adj_chunks, current_water_level,
                        liquid_side_data, num_liquid_sides);
                }
                else if (block->transparent && !block->is_foliage) {
                    // Only pack transparent blocks if within render distance
                    if (render_transparent) {
                        pack_block(i, k, j, lod_scale,
                            c, adj_chunks,
                            transparent_side_data, num_transparent_sides);
                    }
                }
                else if (block->transparent && block->is_foliage) {
                    // Only pack foliage blocks if within render distance
                    if (render_foliage) {
                        pack_block(i, k, j, lod_scale,
                            c, adj_chunks,
                            foliage_side_data, num_foliage_sides);
                    }
                }
                else if (block->is_custom_model) {
                    pack_model(i, k, j, c, custom_model_data, num_custom_verts);
                }
                else {
                    pack_block(i, k, j, lod_scale,
                        c, adj_chunks,
                        opaque_side_data, num_opaque_sides);
                }
            }
        }
    }
}

short calculate_lod(int x, int z, float player_x, float player_z) {
    // get distance from player to center of chunk
    float px = F_WORLD_POS_TO_CHUNK_POS(player_x);
    float pz = F_WORLD_POS_TO_CHUNK_POS(player_z);
    float dx = (float)(x + 0.5f) - px;
    float dz = (float)(z + 0.5f) - pz;
    float dist = sqrtf(dx * dx + dz * dz);

    int lod = 1;
    
    if (dist < CHUNK_RENDER_DISTANCE) {
        return lod;
    }

    int check_dist = CHUNK_RENDER_DISTANCE;
    while (lod < MAX_LOD_BLOCK_SIZE && check_dist < dist) {
        lod *= LOD_SCALING_CONSTANT;
        check_dist = lod * CHUNK_RENDER_DISTANCE;
    }

    return lod;
}

// Check if chunk is within foliage render distance
int is_chunk_in_foliage_distance(int chunk_x, int chunk_z, float player_x, float player_z) {
    float px = F_WORLD_POS_TO_CHUNK_POS(player_x);
    float pz = F_WORLD_POS_TO_CHUNK_POS(player_z);
    float dx = (float)(chunk_x + 0.5f) - px;
    float dz = (float)(chunk_z + 0.5f) - pz;
    float dist = sqrtf(dx * dx + dz * dz);
    return dist <= (float)FOLIAGE_RENDER_DISTANCE;
}

// Check if chunk is within transparent render distance
int is_chunk_in_transparent_distance(int chunk_x, int chunk_z, float player_x, float player_z) {
    float px = F_WORLD_POS_TO_CHUNK_POS(player_x);
    float pz = F_WORLD_POS_TO_CHUNK_POS(player_z);
    float dx = (float)(chunk_x + 0.5f) - px;
    float dz = (float)(chunk_z + 0.5f) - pz;
    float dist = sqrtf(dx * dx + dz * dz);
    return dist <= (float)TRANSPARENT_RENDER_DISTANCE;
}

chunk_mesh* create_chunk_mesh(int x, int z, float player_x, float player_z);

void process_chunk_work_item(chunk_work_item* work) {
    if (work == NULL) {
        return;
    }
    
    // Create chunk mesh for this work item (this also stores it in chunk_packets)
    chunk_mesh* mesh = create_chunk_mesh(work->x, work->z, work->player_x, work->player_z);
    
    work->result_mesh = mesh;
    work->work_complete = 1;
    
    // Free the work item after processing
    // free(work);
}

chunk_mesh* create_chunk_mesh(int x, int z, float player_x, float player_z) {
    chunk_mesh* packet = malloc(sizeof(chunk_mesh));
    assert(packet != NULL && "Failed to allocate memory for packet");

    short lod_scale = calculate_lod(x, z, player_x, player_z);

    // Check render distances for specific block types
    int render_foliage = is_chunk_in_foliage_distance(x, z, player_x, player_z);
    int render_transparent = is_chunk_in_transparent_distance(x, z, player_x, player_z);

    // get chunk and adjacent chunks
    chunk* c = get_chunk(x, z);
    chunk* adj_chunks[4] = {
        get_chunk(x, z - 1),
        get_chunk(x + 1, z),
        get_chunk(x, z + 1),
        get_chunk(x - 1, z)
    };

    // pack chunk data into packet
    int transparent_side_count = 0;
    int foliage_side_count = 0;
    int opaque_side_count = 0;
    int liquid_side_count = 0;
    int custom_model_vert_count = 0;

    // Allocate minimal memory for out-of-distance block types to prevent crashes
    side_instance* opaque_sides = malloc(SIDES_PER_CHUNK * sizeof(side_instance));
    side_instance* transparent_sides = malloc(render_transparent ? SIDES_PER_CHUNK * sizeof(side_instance) : sizeof(side_instance));
    side_instance* liquid_sides = malloc(SIDES_PER_CHUNK * sizeof(side_instance));
    side_instance* foliage_sides = malloc(render_foliage ? SIDES_PER_CHUNK * sizeof(side_instance) : sizeof(side_instance));
    float* custom_model_data = malloc(MODEL_VERTICES_PER_CHUNK * sizeof(float) * FLOATS_PER_MODEL_VERT); // 16 floats per model
    assert(opaque_sides != NULL && "Failed to allocate memory for opaque packet sides");
    assert(transparent_sides != NULL && "Failed to allocate memory for transparent packet sides");
    assert(liquid_sides != NULL && "Failed to allocate memory for liquid packet sides");
    assert(foliage_sides != NULL && "Failed to allocate memory for foliage packet sides");
    assert(custom_model_data != NULL && "Failed to allocate memory for custom model data");

    pack_chunk(c, adj_chunks,
        lod_scale,
        &opaque_sides, &opaque_side_count,
        &transparent_sides, &transparent_side_count,
        &foliage_sides, &foliage_side_count,
        &liquid_sides, &liquid_side_count,
        &custom_model_data, &custom_model_vert_count,
        render_transparent, render_foliage);

    assert(packet != NULL && "Failed to allocate memory for packet");

    packet->x = x;
    packet->z = z;
    packet->lod_scale = lod_scale;
    packet->num_opaque_sides = opaque_side_count;
    packet->num_transparent_sides = transparent_side_count;
    packet->num_liquid_sides = liquid_side_count;
    packet->num_foliage_sides = foliage_side_count;
    packet->num_custom_verts = custom_model_vert_count;
    
    packet->opaque_sides = opaque_sides;
    packet->transparent_sides = transparent_sides;
    packet->liquid_sides = liquid_sides;
    packet->foliage_sides = foliage_sides;
    packet->custom_model_data = custom_model_data;

    // Cache by coordinate + LOD for efficient multi-LOD reuse
    chunk_mesh_key key = {x, z, lod_scale};
    chunk_mesh_lod_map_insert(&chunk_packets, key, *packet);
    
    return packet;
}

chunk_mesh* update_chunk_mesh_at(int x, int z, float player_x, float player_z) {
    // When LOD changes, we generate a NEW mesh at the new LOD
    // The old LOD mesh stays in cache as a fallback
    // This prevents gaps when transitioning between LODs
    
    short new_lod = calculate_lod(x, z, player_x, player_z);
    
    // Check if we already have this new LOD cached
    chunk_mesh_key new_key = {x, z, new_lod};
    chunk_mesh* existing_new_lod = chunk_mesh_lod_map_get(&chunk_packets, new_key);
    if (existing_new_lod != NULL) {
        // Already have the new LOD, just return it
        return existing_new_lod;
    }
    
    // Generate the new LOD mesh
    // This will be cached with the new LOD key
    return create_chunk_mesh(x, z, player_x, player_z);
}

chunk_mesh* update_chunk_mesh(int x, int z, float player_x, float player_z) {
    // Generate/update meshes at the new LOD for this chunk and all adjacent chunks
    // This ensures LOD transitions happen smoothly as the player moves
    
    chunk_coord coords[5] = {
        {x+1, z}, {x-1, z}, {x, z+1}, {x, z-1}, {x, z}
    };
    
    // Update all adjacent chunks at their appropriate LOD
    for (int i = 0; i < 5; i++) {
        chunk_coord coord = coords[i];
        // This will generate or update to the correct LOD for this position
        update_chunk_mesh_at(coord.x, coord.z, player_x, player_z);
    }
    
    // Return the central chunk mesh - try to find the newly generated one at current LOD
    short current_lod = calculate_lod(x, z, player_x, player_z);
    chunk_mesh_key center_key = {x, z, current_lod};
    chunk_mesh* result = chunk_mesh_lod_map_get(&chunk_packets, center_key);
    
    // If not found at exact LOD, try other LODs
    if (result == NULL) {
        for (short lod = 1; lod <= CHUNK_SIZE; lod *= 2) {
            if (lod == current_lod) continue;
            chunk_mesh_key alt_key = {x, z, lod};
            result = chunk_mesh_lod_map_get(&chunk_packets, alt_key);
            if (result != NULL) break;
        }
    }

    return result;
}

chunk_mesh* get_chunk_mesh(int x, int z) {
    // Try to find any LOD version of this chunk in cache
    // Start with LOD 1 (highest quality) and try progressively coarser LODs
    // This allows reuse of previously generated lower-quality meshes
    for (short lod = 1; lod <= CHUNK_SIZE; lod *= 2) {
        chunk_mesh_key key = {x, z, lod};
        chunk_mesh* packet = chunk_mesh_lod_map_get(&chunk_packets, key);
        if (packet != NULL) {
            return packet;
        }
    }

    // If not found at any LOD, queue for generation
    chunk_coord* coord = malloc(sizeof(chunk_coord));
    assert(coord != NULL && "Failed to allocate memory for chunk coord");
    coord->x = x;
    coord->z = z;

    queue_push(&chunk_load_queue, coord, chunk_coord_equals);

    return NULL;
}

void invalidate_chunk_mesh_all_lods(int x, int z) {
    // Remove all LOD versions of a chunk from the cache
    // This is called when a block is modified to force regeneration
    for (short lod = 1; lod <= CHUNK_SIZE; lod *= LOD_SCALING_CONSTANT) {
        chunk_mesh_key key = {x, z, lod};
        chunk_mesh* mesh = chunk_mesh_lod_map_get(&chunk_packets, key);
        if (mesh != NULL) {
            // Free the mesh data
            if (mesh->opaque_sides != NULL) free(mesh->opaque_sides);
            if (mesh->transparent_sides != NULL) free(mesh->transparent_sides);
            if (mesh->liquid_sides != NULL) free(mesh->liquid_sides);
            if (mesh->foliage_sides != NULL) free(mesh->foliage_sides);
            if (mesh->custom_model_data != NULL) free(mesh->custom_model_data);
            
            // Remove from cache
            chunk_mesh_lod_map_remove(&chunk_packets, key);
        }
    }
}

void load_chunk(float player_x, float player_z) {
    if (chunk_worker_pool == NULL) {
        return;
    }
    
    for (int i = 0; i < CHUNK_LOAD_PER_FRAME; i++) {
        chunk_coord* coord = (chunk_coord*)queue_pop(&chunk_load_queue);
        if (coord == NULL) {
            continue;
        }

        // Create work item for the worker pool
        chunk_work_item* work = (chunk_work_item*)malloc(sizeof(chunk_work_item));
        if (work == NULL) {
            free(coord);
            continue;
        }
        
        work->x = coord->x;
        work->z = coord->z;
        work->player_x = player_x;
        work->player_z = player_z;
        work->result_mesh = NULL;
        work->work_complete = 0;
        
        // Submit work to the worker pool
        if (pool_submit_work(chunk_worker_pool, work) != 0) {
            free(work);
            free(coord);
            continue;
        }
        
        free(coord);
    }
}

void wait_chunk_loading(void) {
    if (chunk_worker_pool != NULL) {
        pool_wait_completion(chunk_worker_pool);
    }
}

void queue_chunk_for_sorting(chunk_mesh* packet, int px, int pz) {
    if (packet == NULL) {
        return;
    }

    int dist_sq = (packet->x - px) * (packet->x - px) + (packet->z - pz) * (packet->z - pz);
    if (dist_sq > TRANSPARENT_RENDER_DISTANCE * TRANSPARENT_RENDER_DISTANCE) {
        return;
    }

    queue_push(&sort_queue, packet, chunk_mesh_equals);
}

void sort_chunk() {
    chunk_mesh* packet = (chunk_mesh*)queue_pop(&sort_queue);
    if (packet == NULL) {
        return;
    }

    sort_transparent_sides(packet);
    sort_liquid_sides(packet);
}

// ============================================================================
// Chunk Mesh Builder Implementation
// ============================================================================

chunk_mesh_builder* builder_init(int initial_capacity) {
    chunk_mesh_builder* builder = malloc(sizeof(chunk_mesh_builder));
    if (builder == NULL) {
        return NULL;
    }
    
    builder->capacity = initial_capacity;
    builder->count = 0;
    builder->mesh_array = malloc(initial_capacity * sizeof(chunk_mesh*));
    
    if (builder->mesh_array == NULL) {
        free(builder);
        return NULL;
    }
    
    return builder;
}

void builder_add_mesh(chunk_mesh_builder* b, chunk_mesh* mesh) {
    if (b == NULL || mesh == NULL) {
        return;
    }
    
    // Expand if needed (1.5x growth factor)
    if (b->count >= b->capacity) {
        int new_capacity = (int)(b->capacity * 1.5f);
        chunk_mesh** new_array = realloc(b->mesh_array, new_capacity * sizeof(chunk_mesh*));
        if (new_array == NULL) {
            return;
        }
        b->mesh_array = new_array;
        b->capacity = new_capacity;
    }
    
    b->mesh_array[b->count++] = mesh;
}

void builder_clear(chunk_mesh_builder* b) {
    if (b == NULL) {
        return;
    }
    b->count = 0;
}

void builder_cleanup(chunk_mesh_builder* b) {
    if (b == NULL) {
        return;
    }
    free(b->mesh_array);
    free(b);
}

// ============================================================================
// World Mesh Buffer Implementation
// ============================================================================

world_mesh_buffer* wm_buffer_init(int initial_transparent, int initial_opaque,
                                   int initial_liquid, int initial_foliage,
                                   int initial_custom) {
    world_mesh_buffer* buf = malloc(sizeof(world_mesh_buffer));
    if (buf == NULL) {
        return NULL;
    }
    
    // Allocate buffers
    buf->transparent_data = malloc(initial_transparent * sizeof(int));
    buf->opaque_data = malloc(initial_opaque * sizeof(int));
    buf->liquid_data = malloc(initial_liquid * sizeof(int));
    buf->foliage_data = malloc(initial_foliage * sizeof(int));
    buf->custom_model_data = malloc(initial_custom * sizeof(float));
    
    if (!buf->transparent_data || !buf->opaque_data || !buf->liquid_data ||
        !buf->foliage_data || !buf->custom_model_data) {
        free(buf->transparent_data);
        free(buf->opaque_data);
        free(buf->liquid_data);
        free(buf->foliage_data);
        free(buf->custom_model_data);
        free(buf);
        return NULL;
    }
    
    // Set capacities
    buf->transparent_capacity = initial_transparent;
    buf->opaque_capacity = initial_opaque;
    buf->liquid_capacity = initial_liquid;
    buf->foliage_capacity = initial_foliage;
    buf->custom_capacity = initial_custom;
    
    // Initialize counts to zero
    wm_buffer_reset_counts(buf);
    
    return buf;
}

void wm_buffer_ensure_capacity(world_mesh_buffer* buf, int transparent_needed,
                                int opaque_needed, int liquid_needed,
                                int foliage_needed, int custom_needed) {
    if (buf == NULL) {
        return;
    }
    
    // Reallocate if needed (with 1.5x safety margin)
    if (transparent_needed > buf->transparent_capacity) {
        int new_cap = (int)(transparent_needed * 1.5f);
        buf->transparent_data = realloc(buf->transparent_data, new_cap * sizeof(int));
        buf->transparent_capacity = new_cap;
    }
    
    if (opaque_needed > buf->opaque_capacity) {
        int new_cap = (int)(opaque_needed * 1.5f);
        buf->opaque_data = realloc(buf->opaque_data, new_cap * sizeof(int));
        buf->opaque_capacity = new_cap;
    }
    
    if (liquid_needed > buf->liquid_capacity) {
        int new_cap = (int)(liquid_needed * 1.5f);
        buf->liquid_data = realloc(buf->liquid_data, new_cap * sizeof(int));
        buf->liquid_capacity = new_cap;
    }
    
    if (foliage_needed > buf->foliage_capacity) {
        int new_cap = (int)(foliage_needed * 1.5f);
        buf->foliage_data = realloc(buf->foliage_data, new_cap * sizeof(int));
        buf->foliage_capacity = new_cap;
    }
    
    if (custom_needed > buf->custom_capacity) {
        int new_cap = (int)(custom_needed * 1.5f);
        buf->custom_model_data = realloc(buf->custom_model_data, new_cap * sizeof(float));
        buf->custom_capacity = new_cap;
    }
}

void wm_buffer_reset_counts(world_mesh_buffer* buf) {
    if (buf == NULL) {
        return;
    }
    buf->num_transparent_sides = 0;
    buf->num_opaque_sides = 0;
    buf->num_liquid_sides = 0;
    buf->num_foliage_sides = 0;
    buf->num_custom_verts = 0;
}

void wm_buffer_free(world_mesh_buffer* buf) {
    if (buf == NULL) {
        return;
    }
    free(buf->transparent_data);
    free(buf->opaque_data);
    free(buf->liquid_data);
    free(buf->foliage_data);
    free(buf->custom_model_data);
    free(buf);
}
