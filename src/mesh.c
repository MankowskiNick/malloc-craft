#include <mesh.h>
#include <settings.h>
#include <block.h>
#include <world.h>

DEFINE_HASHMAP(chunk_mesh_map, chunk_coord, chunk_mesh, chunk_hash, chunk_equals);
typedef chunk_mesh_map_hashmap chunk_mesh_map;
chunk_mesh_map chunk_packets;

void m_init() {
    chunk_packets = chunk_mesh_map_init(CHUNK_CACHE_SIZE);
}

void pack_side(int x_0, int y_0, int z_0, uint side, block_type* type, float* side_data) {
    int cube_vertices_offset = side * VERTS_PER_SIDE * CUBE_VERTICES_WIDTH;
    for (int i = 0; i < 6; i++) {
        int index = cube_vertices_offset + i * CUBE_VERTICES_WIDTH;
        float x = x_0 + CUBE_VERTICES[index + 0];
        float y = y_0 + CUBE_VERTICES[index + 1];
        float z = z_0 + CUBE_VERTICES[index + 2];

        float tx = CUBE_VERTICES[index + 3];
        float ty = CUBE_VERTICES[index + 4];

        int opaque_side_data_offset = i * VBO_WIDTH;
        side_data[opaque_side_data_offset + 0] = x;
        side_data[opaque_side_data_offset + 0] = x;
        side_data[opaque_side_data_offset + 0] = x;
        side_data[opaque_side_data_offset + 0] = x;
        side_data[opaque_side_data_offset + 0] = x;
        side_data[opaque_side_data_offset + 1] = y;
        side_data[opaque_side_data_offset + 2] = z;
        side_data[opaque_side_data_offset + 3] = tx;
        side_data[opaque_side_data_offset + 4] = ty;
        side_data[opaque_side_data_offset + 5] = type->face_atlas_coords[side][0] / (float)TEXTURE_ATLAS_SIZE;
        side_data[opaque_side_data_offset + 6] = type->face_atlas_coords[side][1] / (float)TEXTURE_ATLAS_SIZE;
    }
}

void pack_block(
    int x, int y, int z,
    chunk* c,
    chunk* adj_chunks[4], // front, back, left, right
    float** side_data, 
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
        if (new_side_count * SIDE_OFFSET > INITIAL_VBO_SIZE) {
            float* tmp = realloc(*side_data, new_side_count * SIDE_OFFSET * sizeof(float));
            assert(tmp != NULL && "Failed to allocate memory for side data");
            *side_data = tmp;
        }
        block_type* type = c->blocks[x][y][z];

        pack_side(
            world_x, world_y, world_z, 
            side, 
            type,
            (*side_data) + (*num_sides) * SIDE_OFFSET
        );
        (*num_sides)++;
    }
}

void pack_chunk(chunk* c, chunk* adj_chunks[4], 
    float** opaque_side_data, int* num_opaque_sides,
    float** transparent_side_data, int* num_transparent_sides) {
    if (c == NULL) {
        return;
    }

    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (c->blocks[i][k][j] == NULL) {
                    continue;
                }

                if (c->blocks[i][k][j]->transparent) {
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
    int packet_transparent_side_count = 0;
    int packet_opaque_side_count = 0;

    float* packet_opaque_sides = malloc(INITIAL_VBO_SIZE * sizeof(float));
    float* packet_transparent_sides = malloc(INITIAL_VBO_SIZE * sizeof(float));
    assert(packet_opaque_sides != NULL && "Failed to allocate memory for opaque packet sides");
    assert(packet_transparent_sides != NULL && "Failed to allocate memory for transparent packet sides");

    pack_chunk(c, adj_chunks, 
        &packet_opaque_sides, &packet_opaque_side_count,
        &packet_transparent_sides, &packet_transparent_side_count);

    assert(packet != NULL && "Failed to allocate memory for packet");
    packet->opaque_side_data = packet_opaque_sides;
    packet->num_opaque_sides = packet_opaque_side_count;
    packet->transparent_side_data = packet_transparent_sides;
    packet->num_transparent_sides = packet_transparent_side_count;

    chunk_coord coord = {x, z};
    chunk_mesh_map_insert(&chunk_packets, coord, *packet);
    return packet;
}

void update_chunk_mesh_at(int x, int z) {
    chunk_coord coord = {x, z};
    chunk_mesh* packet = chunk_mesh_map_get(&chunk_packets, coord);
    if (packet == NULL) {
        assert(false && "Packet does not exist");
    }

    free(packet->opaque_side_data);
    chunk_mesh_map_remove(&chunk_packets, coord);

    chunk* c = get_chunk(x, z);
    chunk* adj_chunks[4] = {
        get_chunk(x + 1, z),
        get_chunk(x - 1, z),
        get_chunk(x, z - 1),
        get_chunk(x, z + 1)
    };
    int transparent_side_count = 0;
    int opaque_side_count = 0;
    float* transparent_sides = malloc(INITIAL_VBO_SIZE * sizeof(float));
    float* opaque_sides = malloc(INITIAL_VBO_SIZE * sizeof(float));
    assert(transparent_sides != NULL && "Failed to allocate memory for transparent packet sides");
    assert(opaque_sides != NULL && "Failed to allocate memory for opaque packet sides");
    pack_chunk(c, adj_chunks, 
        &opaque_sides, &opaque_side_count, 
        &transparent_sides, &transparent_side_count);

    packet = malloc(sizeof(chunk_mesh));
    assert(packet != NULL && "Failed to allocate memory for packet");

    packet->opaque_side_data = opaque_sides;
    packet->num_opaque_sides = opaque_side_count;
    packet->transparent_side_data = transparent_sides;
    packet->num_transparent_sides = transparent_side_count;
    chunk_mesh_map_insert(&chunk_packets, coord, *packet);
}

void update_chunk_mesh(int x, int z) {
    // update chunk and adjacent ones
    update_chunk_mesh_at(x, z);
    update_chunk_mesh_at(x + 1, z);
    update_chunk_mesh_at(x - 1, z);
    update_chunk_mesh_at(x, z + 1);
    update_chunk_mesh_at(x, z - 1);
}

chunk_mesh* get_chunk_mesh(int x, int z) {
    chunk_coord coord = {x, z};
    chunk_mesh* packet = chunk_mesh_map_get(&chunk_packets, coord);
    if (packet == NULL) {
        packet = create_chunk_mesh(x, z);
    }
    return packet;
}