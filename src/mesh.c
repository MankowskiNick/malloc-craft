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
}

float distance_to_camera(const void* item) {
    side_data* side = (side_data*)item;

    // multiply by -1 to sort in descending order
    return -1.0f * sqrt(
        pow(side->vertices[0].x - m_cam_ref->position[0], 2) +
        pow(side->vertices[0].y - m_cam_ref->position[1], 2) +
        pow(side->vertices[0].z - m_cam_ref->position[2], 2)
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
        block_type* type = c->blocks[x][y][z];

        pack_side(
            world_x, world_y, world_z, 
            side, 
            type,
            &((*chunk_side_data)[*num_sides])  // Correct way
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

    free(packet->opaque_data);
    free(packet->transparent_data);
    chunk_mesh_map_remove(&chunk_packets, coord);

    return create_chunk_mesh(x, z);
}

chunk_mesh* update_chunk_mesh(int x, int z) {
    // update chunk and adjacent ones
    update_chunk_mesh_at(x + 1, z);
    update_chunk_mesh_at(x - 1, z);
    update_chunk_mesh_at(x, z + 1);
    update_chunk_mesh_at(x, z - 1);
    return update_chunk_mesh_at(x, z);
}

chunk_mesh* get_chunk_mesh(int x, int z) {
    chunk_coord coord = {x, z};
    chunk_mesh* packet = chunk_mesh_map_get(&chunk_packets, coord);
    if (packet == NULL) {
        packet = create_chunk_mesh(x, z);
    }
    return packet;
}

void sort_transparent_sides(chunk_mesh* packet) {
    quicksort(packet->transparent_sides, packet->num_transparent_sides, sizeof(side_data), distance_to_camera);
    packet->transparent_data = chunk_mesh_to_float_array(packet->transparent_sides, packet->num_transparent_sides);
}

void mesh_queue_push(chunk_mesh* packet) {

    printf("Pushing packet at %d, %d\n", packet->x, packet->z);
    mesh_queue* prev = NULL;
    mesh_queue* cur = mesh_queue_head;
    int count = 0;
    while (cur != NULL) {
        count++;
        if (cur->x == packet->x && cur->z == packet->z) {
            // Packet already in queue, update it
            cur->packet = packet;
            printf("Found after %d iterations\n", count);
            return;
        }
        
        prev = cur;
        cur = cur->next;
    }
    printf("Not found after %d iterations\n", count);

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

void mesh_queue_pop() {
    if (mesh_queue_head == NULL) {
        return;
    }

    mesh_queue* cur = mesh_queue_head;
    mesh_queue_head = cur->next;
    sort_transparent_sides(cur->packet);
    free(cur);
}