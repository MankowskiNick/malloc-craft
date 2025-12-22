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

short get_adjacent_block_data(int x, int y, int z, short side, chunk* c, chunk* adj) {
    switch(side) {
        case (int)UP:
            if (y + 1 < CHUNK_HEIGHT) {
                return c->blocks[x][y + 1][z];
            }
            break;
        case (int)DOWN:
            if (y - 1 >= 0) {
                return c->blocks[x][y - 1][z];
            }
            break;
        case (int)WEST:
            if (x + 1 < CHUNK_SIZE) {
                return c->blocks[x + 1][y][z];
            }
            else if (adj != NULL) {
                return adj->blocks[0][y][z];
            }
            break;
        case (int)EAST:
            if (x - 1 >= 0) {
                return c->blocks[x - 1][y][z];
            }
            else if (adj != NULL) {
                return adj->blocks[CHUNK_SIZE - 1][y][z];
            }
            break;
        case (int)NORTH:
            if (z - 1 >= 0) {
                return c->blocks[x][y][z - 1];
            }
            else if (adj != NULL) {
                return adj->blocks[x][y][CHUNK_SIZE - 1];
            }
            break;
        case (int)SOUTH:
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
    return 0; // air block data
}

short get_adjacent_block(int x, int y, int z, short side, chunk* c, chunk* adj) {
    short block_data = get_adjacent_block_data(x, y, z, side, c, adj);
    short block_id = 0;
    get_block_info(block_data, &block_id, NULL, NULL, NULL);
    return block_id;
}

// TODO: this function is messy, clean it up
void get_side_visible(
    int x, int y, int z,
    short side,
    chunk* c,
    chunk* adj,
    int* visible_out,
    int* underwater_out,
    int* water_level_out
) {
    // calculate adjacent block
    short adjacent_id = get_adjacent_block(x, y, z, side, c, adj);

    short current_id = 0;

    short current_water_level = 0;
    get_block_info(c->blocks[x][y][z], &current_id, NULL, NULL, &current_water_level);
    block_type* current = get_block_type(current_id);

    // calculate visibility
    block_type* adjacent = get_block_type(adjacent_id);
    uint visible = adjacent_id == get_block_id("air") || get_block_type(adjacent_id)->transparent != current->transparent;

    // check if we are underwater
    if (adjacent_id == get_block_id("water")) {
        *underwater_out = 1;
    }

    // get water level from adjacent block
    short adj_water_level = 0;
    if (adjacent_id == get_block_id("water")) {
        short adj_block_data = get_adjacent_block_data(x, y, z, side, c, adj);
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

void pack_side(int x_0, int y_0, int z_0, 
        short side, 
        short orientation, 
        short rot, 
        short type, 
        short water_level, 
        bool underwater, 
        side_instance* data) {
    data->x = x_0;
    data->y = y_0;
    data->z = z_0;
    data->side = side;
    data->water_level = water_level;
    data->underwater = underwater ? 1 : 0;

    // block specific data
    block_type* block = get_block_type(type);
    data->orientation = block->oriented ? orientation : (short)DOWN;

    short display_side = get_rotated_side(side, rot);
    if (!block->is_custom_model && !block->is_foliage && block->oriented) {
        display_side = get_converted_side(side, orientation);
    }
    data->atlas_x = block->face_atlas_coords[display_side][0];
    data->atlas_y = block->face_atlas_coords[display_side][1];
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

    short block_data = c->blocks[x][y][z];
    short block_id = 0;
    short orientation = 0;
    short rot = 0;
    short current_water_level = 0;
    get_block_info(block_data, &block_id, &orientation, &rot, &current_water_level);

    for (int side = 0; side < 6; side++) {
        chunk* adj = NULL;
        if (side < 4) {
            adj = adj_chunks[side];
        }

        int visible = 0;
        int underwater = 0;
        int adj_water_level = 0;
        get_side_visible(x, y, z, side, c, adj, &visible, &underwater, &adj_water_level);
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
        short water_level_to_use = block->liquid ? current_water_level : (short)adj_water_level;

        pack_side(
            world_x, world_y, world_z,
            side,
            orientation, rot,
            block_id,
            water_level_to_use,
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
                get_block_info(c->blocks[i][k][j], &block_id, NULL, NULL, NULL);
                if (block_id == get_block_id("air")) {
                    continue;
                }

                block_type* block = get_block_type(block_id);
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
    sort_liquid_sides(packet);
}
