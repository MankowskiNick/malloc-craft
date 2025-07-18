#include <mesh.h>
#include <settings.h>
#include <block.h>
#include <world.h>
#include <camera.h>
#include <chunk_mesh.h>
#include <queue.h>
#include <pthread.h>

DEFINE_HASHMAP(chunk_mesh_map, chunk_coord, chunk_mesh, chunk_hash, chunk_equals);
typedef chunk_mesh_map_hashmap chunk_mesh_map;
chunk_mesh_map chunk_packets;

// Mutexes for shared structures
pthread_mutex_t chunk_packets_mutex;
pthread_mutex_t sort_queue_mutex;
pthread_mutex_t chunk_load_queue_mutex;

queue_node* sort_queue = NULL;
queue_node* chunk_load_queue = NULL;

void m_init(camera* camera) {
    chunk_packets = chunk_mesh_map_init(CHUNK_CACHE_SIZE);
    queue_init(&sort_queue);
    queue_init(&chunk_load_queue);
    chunk_mesh_init(camera);

    pthread_mutex_init(&chunk_packets_mutex, NULL);
    pthread_mutex_init(&sort_queue_mutex, NULL);
    pthread_mutex_init(&chunk_load_queue_mutex, NULL);
}

void m_cleanup() {
    chunk_mesh_map_free(&chunk_packets);
    queue_cleanup(&sort_queue);
    queue_cleanup(&chunk_load_queue);

    pthread_mutex_destroy(&chunk_packets_mutex);
    pthread_mutex_destroy(&sort_queue_mutex);
    pthread_mutex_destroy(&chunk_load_queue_mutex);
}

short get_adjacent_block(int x, int y, int z, short side, chunk* c, chunk* adj) {
    switch(side) {
        case (int)TOP:
            if (y + 1 < CHUNK_HEIGHT) {
                return c->blocks[x][y + 1][z];
            }
            break;
        case (int)BOTTOM:
            if (y - 1 >= 0) {
                return c->blocks[x][y - 1][z];
            }
            break;
        case (int)FRONT:
            if (x + 1 < CHUNK_SIZE) {
                return c->blocks[x + 1][y][z];
            }
            else if (adj != NULL) {
                return adj->blocks[0][y][z];
            }
            break;
        case (int)BACK:
            if (x - 1 >= 0) {
                return c->blocks[x - 1][y][z];
            }
            else if (adj != NULL) {
                return adj->blocks[CHUNK_SIZE - 1][y][z];
            }
            break;
        case (int)LEFT:
            if (z - 1 >= 0) {
                return c->blocks[x][y][z - 1];
            }
            else if (adj != NULL) {
                return adj->blocks[x][y][CHUNK_SIZE - 1];
            }
            break;
        case (int)RIGHT:
            if (z + 1 < CHUNK_SIZE) {
                return c->blocks[x][y][z + 1];
            }
            else if (adj != NULL) {
                return adj->blocks[x][y][0];
            }
            break;
        default:
            break;
    }
    return AIR;
}

void get_side_visible(
    int x, int y, int z,
    short side, 
    chunk* c,
    chunk* adj,
    int* visible_out,
    int* underwater_out
) {
    // calculate adjacent block
    short adjacent_id = get_adjacent_block(x, y, z, side, c, adj);

    short current_id = c->blocks[x][y][z];
    block_type* current = get_block_type(current_id);

    // calculate visibility 
    block_type* adjacent = get_block_type(adjacent_id);
    uint visible = adjacent_id == AIR || get_block_type(adjacent_id)->transparent != current->transparent;

    // check if we are underwater
    if (adjacent_id == WATER) {
        *underwater_out = 1;
    }

    // make sure transparent neighbors are visible
    if (adjacent != NULL && adjacent->transparent != current->transparent) {
        visible = 1;
    }

    if (adjacent != NULL 
        && (adjacent->transparent && current->transparent)
        && adjacent->id != current->id) {
        visible = 1;
    }

    // dont render sides that we can't see
    switch(side) {
        case (int)TOP:
            visible = visible && y < CHUNK_HEIGHT;
            break;
        case (int)BOTTOM:
            visible = visible && y > 0;
            break;
        default:
            break;
    }

    *visible_out = visible;
    // *underwater_out = underwater;
}

void pack_side(int x_0, int y_0, int z_0, short side, short type, uint underwater, side_instance* data) {
    data->x = x_0;
    data->y = y_0;
    data->z = z_0;
    data->side = side;
    data->underwater = (short)underwater;
    block_type* block = get_block_type(type);
    data->atlas_x = block->face_atlas_coords[side][0];
    data->atlas_y = block->face_atlas_coords[side][1];
}

void pack_block(
    int x, int y, int z,
    chunk* c,
    chunk* adj_chunks[4], // front, back, left, right
    side_instance** chunk_side_data, 
    int* num_sides) {
    
    int world_x = x + (CHUNK_SIZE * c->x);
    int world_y = y;
    int world_z = z + (CHUNK_SIZE * c->z);

    for (int side = 0; side < 6; side++) {
        chunk* adj = NULL;
        if (side < 4) {
            adj = adj_chunks[side];
        }

        int visible = 0;
        int underwater = 0;
        get_side_visible(x, y, z, side, c, adj, &visible, &underwater);
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

        short block_id = c->blocks[x][y][z];

        pack_side(
            world_x, world_y, world_z, 
            side, 
            block_id,
            underwater,
            &((*chunk_side_data)[*num_sides])
        );
        (*num_sides)++;
    }
}

void pack_chunk(chunk* c, chunk* adj_chunks[4], 
    side_instance** opaque_side_data, int* num_opaque_sides,
    side_instance** transparent_side_data, int* num_transparent_sides,
    side_instance** liquid_side_data, int* num_liquid_sides) {
    if (c == NULL) {
        return;
    }

    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (c->blocks[i][k][j] == AIR) {
                    continue;
                }

                short block_id = c->blocks[i][k][j];
                block_type* block = get_block_type(block_id);

                if (block->liquid) {
                    pack_block(i, k, j, c, adj_chunks, 
                        liquid_side_data, num_liquid_sides);
                }
                else if (block->transparent) {
                    pack_block(i, k, j, c, adj_chunks, 
                        transparent_side_data, num_transparent_sides);
                }
                else {
                    pack_block(i, k, j, c, adj_chunks, 
                        opaque_side_data, num_opaque_sides);
                }
            }
        }
    }
}

chunk_mesh* create_chunk_mesh(int x, int z) {
    chunk_mesh* packet = malloc(sizeof(chunk_mesh));
    assert(packet != NULL && "Failed to allocate memory for packet");

    // get chunk and adjacent chunks
    chunk* c = get_chunk(x, z);
    chunk* adj_chunks[4] = {
        get_chunk(x, z + 1),
        get_chunk(x, z - 1),
        get_chunk(x - 1, z),
        get_chunk(x + 1, z)
    };

    // pack chunk data into packet
    int transparent_side_count = 0;
    int opaque_side_count = 0;
    int liquid_side_count = 0;

    side_instance* opaque_sides = malloc(SIDES_PER_CHUNK * sizeof(side_instance));
    side_instance* transparent_sides = malloc(SIDES_PER_CHUNK * sizeof(side_instance));
    side_instance* liquid_sides = malloc(SIDES_PER_CHUNK * sizeof(side_instance));
    assert(opaque_sides != NULL && "Failed to allocate memory for opaque packet sides");
    assert(transparent_sides != NULL && "Failed to allocate memory for transparent packet sides");
    assert(liquid_sides != NULL && "Failed to allocate memory for liquid packet sides");

    pack_chunk(c, adj_chunks, 
        &opaque_sides, &opaque_side_count,
        &transparent_sides, &transparent_side_count,
        &liquid_sides, &liquid_side_count);

    assert(packet != NULL && "Failed to allocate memory for packet");

    packet->x = x;
    packet->z = z;
    packet->num_opaque_sides = opaque_side_count;
    packet->num_transparent_sides = transparent_side_count;
    packet->num_liquid_sides = liquid_side_count;
    
    packet->opaque_sides = opaque_sides;
    packet->transparent_sides = transparent_sides;
    packet->liquid_sides = liquid_sides;

    chunk_coord coord = {x, z};
    chunk_mesh_map_insert(&chunk_packets, coord, *packet);
    
    return packet;
}

chunk_mesh* update_chunk_mesh_at(int x, int z) {
    chunk_coord coord = {x, z};
    chunk_mesh* packet = chunk_mesh_map_get(&chunk_packets, coord);
    if (packet == NULL) {
        assert(false && "Packet does not exist");
    }
    queue_remove(&sort_queue, packet, chunk_mesh_equals);

    free(packet->opaque_sides);
    packet->opaque_sides = NULL;
    free(packet->transparent_sides);
    packet->transparent_sides = NULL;

    chunk_mesh_map_remove(&chunk_packets, coord);

    return create_chunk_mesh(x, z);
}

chunk_mesh* update_chunk_mesh(int x, int z) {
    chunk_coord coords[] = {
        {x+1, z}, {x-1, z}, {x, z+1}, {x, z-1}, {x, z}
    };
    
    // Check which chunks exist first
    for (int i = 0; i < 5; i++) {
        chunk_coord coord = coords[i];
        int exists = chunk_mesh_map_get(&chunk_packets, coord) != NULL;
        if (exists) {
            update_chunk_mesh_at(coord.x, coord.z);
        }
    }
    
    // Return the central chunk mesh
    chunk_coord center = {x, z};
    chunk_mesh* result = chunk_mesh_map_get(&chunk_packets, center);

    return result;
}

chunk_mesh* get_chunk_mesh(int x, int z) {
    chunk_coord* coord = malloc(sizeof(chunk_coord));
    assert(coord != NULL && "Failed to allocate memory for chunk coord");
    coord->x = x;
    coord->z = z;

    chunk_mesh* packet = chunk_mesh_map_get(&chunk_packets, *coord);

    if (packet != NULL) {
        free(coord);
        return packet;
    }

    queue_push(&chunk_load_queue, coord, chunk_coord_equals);

    return NULL;
}

void load_chunk() {
    for (int i = 0; i < CHUNK_LOAD_PER_FRAME; i++) {
        chunk_coord* coord = (chunk_coord*)queue_pop(&chunk_load_queue);
        if (coord == NULL) {
            continue;
        }
        create_chunk_mesh(coord->x, coord->z);
        free(coord);
    }
}

void queue_chunk_for_sorting(chunk_mesh* packet) {
    queue_push(&sort_queue, packet, chunk_mesh_equals);
}

void sort_chunk() {
    chunk_mesh* packet = (chunk_mesh*)queue_pop(&sort_queue);
    if (packet == NULL) {
        return;
    }

    sort_transparent_sides(packet);
}
