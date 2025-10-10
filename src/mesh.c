#include <mesh.h>
#include <settings.h>
#include <block.h>
#include <world.h>
#include <camera.h>
#include <chunk_mesh.h>
#include <queue.h>
#include <pthread.h>
#include <assert.h>
#include <blockbench_loader.h>

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
    short block_id = 0;
    switch(side) {
        case (int)UP:
            if (y + 1 < CHUNK_HEIGHT) {
                get_block_info(c->blocks[x][y + 1][z], &block_id, NULL);
                return block_id;
            }
            break;
        case (int)DOWN:
            if (y - 1 >= 0) {
                get_block_info(c->blocks[x][y - 1][z], &block_id, NULL);
                return block_id;
            }
            break;
        case (int)EAST:
            if (x + 1 < CHUNK_SIZE) {
                get_block_info(c->blocks[x + 1][y][z], &block_id, NULL);
                return block_id;
            }
            else if (adj != NULL) {
                get_block_info(adj->blocks[0][y][z], &block_id, NULL);
                return block_id;
            }
            break;
        case (int)WEST:
            if (x - 1 >= 0) {
                get_block_info(c->blocks[x - 1][y][z], &block_id, NULL);
                return block_id;
            }
            else if (adj != NULL) {
                get_block_info(adj->blocks[CHUNK_SIZE - 1][y][z], &block_id, NULL);
                return block_id;
            }
            break;
        case (int)NORTH:
            if (z - 1 >= 0) {
                get_block_info(c->blocks[x][y][z - 1], &block_id, NULL);
                return block_id;
            }
            else if (adj != NULL) {
                get_block_info(adj->blocks[x][y][CHUNK_SIZE - 1], &block_id, NULL);
                return block_id;
            }
            break;
        case (int)SOUTH:
            if (z + 1 < CHUNK_SIZE) {
                get_block_info(c->blocks[x][y][z + 1], &block_id, NULL);
                return block_id;
            }
            else if (adj != NULL) {
                get_block_info(adj->blocks[x][y][0], &block_id, NULL);
                return block_id;
            }
            break;
        default:
            break;
    }
    return get_block_id("air");
}

// TODO: this function is messy, clean it up
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

    short current_id;
    get_block_info(c->blocks[x][y][z], &current_id, NULL);
    block_type* current = get_block_type(current_id);

    // calculate visibility 
    block_type* adjacent = get_block_type(adjacent_id);
    uint visible = adjacent_id == get_block_id("air") || get_block_type(adjacent_id)->transparent != current->transparent;

    // check if we are underwater
    if (adjacent_id == get_block_id("water")) {
        *underwater_out = 1;
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

        short block_id = 0;
        get_block_info(c->blocks[x][y][z], &block_id, NULL);

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

void pack_model(
    int x, int y, int z,
    chunk* c,
    float** custom_model_data,
    int* num_custom_verts
) {
    short block_id = 0;
    get_block_info(c->blocks[x][y][z], &block_id, NULL);
    block_type* block = get_block_type(block_id);
    if (block == NULL || !block->is_custom_model) {
        return;
    }

    // reference blockbench model data hashmap based on model name
    blockbench_model* model = get_blockbench_model(block->model);
    if (model == NULL) {
        return;
    }

    int new_vert_count = (*num_custom_verts) + model->index_count;
    if (new_vert_count > MODEL_VERTICES_PER_CHUNK) {
        float* tmp = realloc(*custom_model_data, new_vert_count * sizeof(float) * FLOATS_PER_MODEL_VERT);
        assert(tmp != NULL && "Failed to allocate memory for custom model data");
        *custom_model_data = tmp;
    }

    // copy model data into chunk mesh data
    for (int i = 0; i < model->index_count; i++) {
        int dest_idx = (*num_custom_verts + i) * FLOATS_PER_MODEL_VERT;

        float dest_x = (float)x + (float)(c->x * CHUNK_SIZE);
        float dest_y = (float)y;
        float dest_z = (float)z + (float)(c->z * CHUNK_SIZE);
        blockbench_vertex vert = model->vertices[model->indices[i]];

        // position
        (*custom_model_data)[dest_idx + 0] = dest_x + vert.position[0];
        (*custom_model_data)[dest_idx + 1] = dest_y + vert.position[1];
        (*custom_model_data)[dest_idx + 2] = dest_z + vert.position[2];

        // normal
        (*custom_model_data)[dest_idx + 3] = vert.normal[0];
        (*custom_model_data)[dest_idx + 4] = vert.normal[1];
        (*custom_model_data)[dest_idx + 5] = vert.normal[2];

        // uv
        (*custom_model_data)[dest_idx + 6] = vert.uv[0];
        (*custom_model_data)[dest_idx + 7] = vert.uv[1];
    }

    *num_custom_verts = new_vert_count;
}

void pack_chunk(chunk* c, chunk* adj_chunks[4], 
    side_instance** opaque_side_data, int* num_opaque_sides,
    side_instance** transparent_side_data, int* num_transparent_sides,
    side_instance** foliage_side_data, int* num_foliage_sides,
    side_instance** liquid_side_data, int* num_liquid_sides,
    float** custom_model_data, int* num_custom_verts) {
    if (c == NULL) {
        return;
    }

    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                short block_id = 0;
                get_block_info(c->blocks[i][k][j], &block_id, NULL);
                if (block_id == get_block_id("air")) {
                    continue;
                }

                block_type* block = get_block_type(block_id);

                if (block_id == 20) {
                    int debug = 1;
                }

                if (block->liquid) {
                    pack_block(i, k, j, c, adj_chunks, 
                        liquid_side_data, num_liquid_sides);
                }
                else if (block->transparent && !block->is_foliage) {
                    pack_block(i, k, j, c, adj_chunks, 
                        transparent_side_data, num_transparent_sides);
                }
                else if (block->transparent && block->is_foliage) {
                    pack_block(i, k, j, c, adj_chunks, 
                        foliage_side_data, num_foliage_sides);
                }
                else if (block->is_custom_model) {
                    pack_model(i, k, j, c, custom_model_data, num_custom_verts);
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
    int foliage_side_count = 0;
    int opaque_side_count = 0;
    int liquid_side_count = 0;
    int custom_model_vert_count = 0;

    side_instance* opaque_sides = malloc(SIDES_PER_CHUNK * sizeof(side_instance));
    side_instance* transparent_sides = malloc(SIDES_PER_CHUNK * sizeof(side_instance));
    side_instance* liquid_sides = malloc(SIDES_PER_CHUNK * sizeof(side_instance));
    side_instance* foliage_sides = malloc(SIDES_PER_CHUNK * sizeof(side_instance));
    float* custom_model_data = malloc(MODEL_VERTICES_PER_CHUNK * sizeof(float) * FLOATS_PER_MODEL_VERT); // 16 floats per model
    assert(opaque_sides != NULL && "Failed to allocate memory for opaque packet sides");
    assert(transparent_sides != NULL && "Failed to allocate memory for transparent packet sides");
    assert(liquid_sides != NULL && "Failed to allocate memory for liquid packet sides");
    assert(foliage_sides != NULL && "Failed to allocate memory for foliage packet sides");
    assert(custom_model_data != NULL && "Failed to allocate memory for custom model data");

    pack_chunk(c, adj_chunks, 
        &opaque_sides, &opaque_side_count,
        &transparent_sides, &transparent_side_count,
        &foliage_sides, &foliage_side_count,
        &liquid_sides, &liquid_side_count,
        &custom_model_data, &custom_model_vert_count);

    assert(packet != NULL && "Failed to allocate memory for packet");

    packet->x = x;
    packet->z = z;
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
    free(packet->foliage_sides);
    packet->foliage_sides = NULL;
    free(packet->liquid_sides);
    packet->liquid_sides = NULL;
    free(packet->custom_model_data);
    packet->custom_model_data = NULL;

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
