#include <mesh.h>
#include <settings.h>
#include <block.h>
#include <world.h>
#include <camera.h>
#include <sort.h>

DEFINE_HASHMAP(chunk_mesh_map, chunk_coord, chunk_mesh, chunk_hash, chunk_equals);
typedef chunk_mesh_map_hashmap chunk_mesh_map;
chunk_mesh_map chunk_packets;

typedef struct mesh_queue {
    int x, z;
    chunk_mesh* packet;
    struct mesh_queue* next;
} mesh_queue;

mesh_queue* mesh_queue_head = NULL;

camera* m_cam_ref;

void m_init(camera* camera) {
    chunk_packets = chunk_mesh_map_init(CHUNK_CACHE_SIZE);
    m_cam_ref = camera;
}

void m_cleanup() {
    chunk_mesh_map_free(&chunk_packets);

    mesh_queue* cur = mesh_queue_head;
    while (cur != NULL) {
        mesh_queue* next = cur->next;
        free(cur);
        cur = next;
    }
    mesh_queue_head = NULL;
}

float distance_to_camera(const void* item) {
    side_data* side = (side_data*)item;

    float sx = 0.0f;
    float sy = 0.0f;
    float sz = 0.0f;
    for (int i = 0; i < VERTS_PER_SIDE; i++) {
        sx += side->vertices[i].x;
        sy += side->vertices[i].y;
        sz += side->vertices[i].z;
    }
    sx /= VERTS_PER_SIDE;
    sy /= VERTS_PER_SIDE;
    sz /= VERTS_PER_SIDE;

    // multiply by -1 to sort in descending order
    return -1.0f * sqrt(
        pow(sx - m_cam_ref->position[0], 2) +
        pow(sy - m_cam_ref->position[1], 2) +
        pow(sz - m_cam_ref->position[2], 2)
    );
}

float* chunk_mesh_to_float_array(side_data* sides, int num_sides) {
    float* data = malloc(num_sides * SIDE_OFFSET * sizeof(float));
    assert(data != NULL && "Failed to allocate memory for float array");

    for (int i = 0; i < num_sides; i++) {
        for (int j = 0; j < VERTS_PER_SIDE; j++) {
            int index = i * SIDE_OFFSET + j * VBO_WIDTH;
            data[index + 0] = sides[i].vertices[j].x;
            data[index + 1] = sides[i].vertices[j].y;
            data[index + 2] = sides[i].vertices[j].z;
            data[index + 3] = sides[i].vertices[j].tx;
            data[index + 4] = sides[i].vertices[j].ty;
            data[index + 5] = sides[i].vertices[j].atlas_x;
            data[index + 6] = sides[i].vertices[j].atlas_y;
        }
    }
    return data;
}

void sort_transparent_sides(chunk_mesh* packet) {
    quicksort(packet->transparent_sides, packet->num_transparent_sides, sizeof(side_data), distance_to_camera);
    if (packet->transparent_data != NULL) {
        free(packet->transparent_data);
        packet->transparent_data = NULL;
    }
    packet->transparent_data = chunk_mesh_to_float_array(packet->transparent_sides, packet->num_transparent_sides);
}

void mesh_queue_push(chunk_mesh* packet) {
    mesh_queue* prev = NULL;
    mesh_queue* cur = mesh_queue_head;
    int count = 0;
    while (cur != NULL) {
        count++;
        if (cur->x == packet->x && cur->z == packet->z) {
            cur->packet = packet;
            return;
        }
        
        prev = cur;
        cur = cur->next;
    }

    // Packet not in queue, add it
    mesh_queue* new_node = malloc(sizeof(mesh_queue));
    assert(new_node != NULL && "Failed to allocate memory for mesh queue node");
    new_node->packet = packet;
    new_node->x = packet->x;
    new_node->z = packet->z;
    new_node->next = NULL;

    if (prev == NULL) {
        mesh_queue_head = new_node;
    }
    else {
        prev->next = new_node;
    }
}

void mesh_queue_remove(chunk_mesh* packet) {
    if (mesh_queue_head == NULL) {
        return;
    }

    mesh_queue* prev = NULL;
    mesh_queue* cur = mesh_queue_head;
    while (cur != NULL) {
        if (cur->x == packet->x && cur->z == packet->z) {
            if (prev == NULL) {
                mesh_queue_head = cur->next;
            }
            else {
                prev->next = cur->next;
            }
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

void mesh_queue_pop() {
    if (mesh_queue_head == NULL) {
        return;
    }

    mesh_queue* cur = mesh_queue_head;
    mesh_queue_head = cur->next;
    sort_transparent_sides(cur->packet);
    free(cur);
}

short get_adjacent_block(int x, int y, int z, uint side, chunk* c, chunk* adj) {
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

int get_side_visible(
    int x, int y, int z,
    uint side, 
    chunk* c,
    chunk* adj
) {
    uint visible = 0;

    // calculate adjacent block
    short adjacent_id = get_adjacent_block(x, y, z, side, c, adj);

    short current_id = c->blocks[x][y][z];
    block_type* current = get_block_type(current_id);

    // calculate visibility 
    block_type* adjacent = get_block_type(adjacent_id);
    visible = adjacent_id == AIR || get_block_type(adjacent_id)->transparent != current->transparent;

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

    return visible;
}


void pack_side(int x_0, int y_0, int z_0, uint side, block_type* type, side_data* data) {
    int cube_vertices_offset = side * VERTS_PER_SIDE * CUBE_VERTICES_WIDTH;
    for (int i = 0; i < 6; i++) {
        int index = cube_vertices_offset + i * CUBE_VERTICES_WIDTH;
        float x = x_0 + CUBE_VERTICES[index + 0];
        float y = y_0 + CUBE_VERTICES[index + 1];
        float z = z_0 + CUBE_VERTICES[index + 2];

        float tx = CUBE_VERTICES[index + 3];
        float ty = CUBE_VERTICES[index + 4];

        data->vertices[i].x = x;
        data->vertices[i].y = y;
        data->vertices[i].z = z;
        data->vertices[i].tx = tx;
        data->vertices[i].ty = ty;
        data->vertices[i].atlas_x = type->face_atlas_coords[side][0] / (float)TEXTURE_ATLAS_SIZE;
        data->vertices[i].atlas_y = type->face_atlas_coords[side][1] / (float)TEXTURE_ATLAS_SIZE;
    }
}

void pack_block(
    int x, int y, int z,
    chunk* c,
    chunk* adj_chunks[4], // front, back, left, right
    side_data** chunk_side_data, 
    int* num_sides) {
    
    int world_x = x + (CHUNK_SIZE * c->x);
    int world_y = y;
    int world_z = z + (CHUNK_SIZE * c->z);

    for (int side = 0; side < 6; side++) {
        chunk* adj = NULL;
        if (side < 4) {
            adj = adj_chunks[side];
        }

        if (!get_side_visible(x, y, z, side, c, adj)) {
            continue;
        }

        int new_side_count = (*num_sides) + 1;

        // check if we need to reallocate memory
        if (new_side_count > SIDES_PER_CHUNK) {
            side_data* tmp = realloc(*chunk_side_data, new_side_count * sizeof(side_data));
            assert(tmp != NULL && "Failed to allocate memory for side data");
            *chunk_side_data = tmp;
        }

        short block_id = c->blocks[x][y][z];
        block_type* type = get_block_type(block_id);

        pack_side(
            world_x, world_y, world_z, 
            side, 
            type,
            &((*chunk_side_data)[*num_sides])
        );
        (*num_sides)++;
    }
}

void pack_chunk(chunk* c, chunk* adj_chunks[4], 
    side_data** opaque_side_data, int* num_opaque_sides,
    side_data** transparent_side_data, int* num_transparent_sides) {
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

                if (block->transparent) {
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
        get_chunk(x + 1, z),
        get_chunk(x - 1, z),
        get_chunk(x, z - 1),
        get_chunk(x, z + 1)
    };

    // pack chunk data into packet
    int transparent_side_count = 0;
    int opaque_side_count = 0;

    side_data* opaque_sides = malloc(SIDES_PER_CHUNK * sizeof(side_data));
    side_data* transparent_sides = malloc(SIDES_PER_CHUNK * sizeof(side_data));
    assert(opaque_sides != NULL && "Failed to allocate memory for opaque packet sides");
    assert(transparent_sides != NULL && "Failed to allocate memory for transparent packet sides");

    pack_chunk(c, adj_chunks, 
        &opaque_sides, &opaque_side_count,
        &transparent_sides, &transparent_side_count);

    assert(packet != NULL && "Failed to allocate memory for packet");

    packet->x = x;
    packet->z = z;
    packet->num_opaque_sides = opaque_side_count;
    packet->num_transparent_sides = transparent_side_count;
    packet->opaque_data = chunk_mesh_to_float_array(opaque_sides, opaque_side_count);
    packet->transparent_data = chunk_mesh_to_float_array(transparent_sides, transparent_side_count);
    packet->opaque_sides = opaque_sides;
    packet->transparent_sides = transparent_sides;

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

    mesh_queue_remove(packet);
    free(packet->opaque_data);
    packet->opaque_data = NULL;
    free(packet->transparent_data);
    packet->transparent_data = NULL;
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
        if (chunk_mesh_map_get(&chunk_packets, coord)) {
            update_chunk_mesh_at(coord.x, coord.z);
        }
    }
    
    // Return the central chunk mesh
    chunk_coord center = {x, z};
    
    return chunk_mesh_map_get(&chunk_packets, center);
}

chunk_mesh* get_chunk_mesh(int x, int z) {
    chunk_coord coord = {x, z};
    chunk_mesh* packet = chunk_mesh_map_get(&chunk_packets, coord);
    if (packet == NULL) {
        packet = create_chunk_mesh(x, z);
    }
    return packet;
}