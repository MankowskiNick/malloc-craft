#include <render.h>
#include <block.h>
#include <chunk.h>
#include <world.h>
#include <vbo.h>
#include <vao.h>
#include <texture.h>
#include <assert.h>
#include <hashmap.h>

#include <settings.h>

#define VBO_WIDTH 7
#define VERTS_PER_SIDE 6
#define SIDE_OFFSET VERTS_PER_SIDE * VBO_WIDTH

#define INITIAL_VBO_SIZE 2000 * SIDE_OFFSET

VAO vao;
VBO vbo;
shader_program program;

typedef struct {
    float* side_data;
    int num_sides;
} chunk_packet;

DEFINE_HASHMAP(chunk_packet_map, chunk_coord, chunk_packet, chunk_hash, chunk_equals);
typedef chunk_packet_map_hashmap chunk_packet_map;

chunk_packet_map packets;

void r_init(shader_program* program) {
    glEnable(GL_DEPTH_TEST);

    #ifdef WIREFRAME
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    #endif

    // buffers
    vao = create_vao();
    bind_vao(vao);
    vbo = create_vbo(GL_STATIC_DRAW);

    // shaders and program
    shader frag_shader = create_shader("res/shaders/shader.frag", GL_FRAGMENT_SHADER);
    shader vert_shader = create_shader("res/shaders/shader.vert", GL_VERTEX_SHADER);

    *program = create_program(vert_shader, frag_shader);
    use_program(*program);

    delete_shader(frag_shader);
    delete_shader(vert_shader);

    // texture
    t_init();

    // world
    w_init();

    // initialize chunk packet map
    packets = chunk_packet_map_init(CHUNK_CACHE_SIZE);

    glViewport(0, 0, WIDTH, HEIGHT);
}

void r_cleanup() {
    delete_vao(vao);
    delete_vbo(vbo);
    delete_program(program);
    t_cleanup();
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

        int side_data_offset = i * VBO_WIDTH;
        side_data[side_data_offset + 0] = x;
        side_data[side_data_offset + 1] = y;
        side_data[side_data_offset + 2] = z;
        side_data[side_data_offset + 3] = tx;
        side_data[side_data_offset + 4] = ty;
        side_data[side_data_offset + 5] = type->face_atlas_coords[side][0] / (float)TEXTURE_ATLAS_SIZE;
        side_data[side_data_offset + 6] = type->face_atlas_coords[side][1] / (float)TEXTURE_ATLAS_SIZE;
    }
}

void pack_block(
    int x, int y, int z,
    chunk* c,
    chunk* adj_chunks[4], // front, back, left, right
    float** side_data, 
    int* num_sides
) {
    
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
        if (new_side_count > INITIAL_VBO_SIZE) {
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

void pack_chunk(chunk* c, chunk* adj_chunks[4], float** side_data, int* num_sides) {
    if (c == NULL) {
        return;
    }

    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (c->blocks[i][k][j] == NULL) {
                    continue;
                }
                pack_block(i, k, j, c, adj_chunks, side_data, num_sides);
            }
        }
    }
}

void render_packet(chunk_packet* packet) {
    bind_vao(vao);
    buffer_data(vbo, GL_STATIC_DRAW, packet->side_data, packet->num_sides * SIDE_OFFSET * sizeof(float));
    add_attrib(&vbo, 0, 3, 0, VBO_WIDTH * sizeof(float));
    add_attrib(&vbo, 1, 2, 3 * sizeof(float), VBO_WIDTH * sizeof(float));
    add_attrib(&vbo, 2, 2, 5 * sizeof(float), VBO_WIDTH * sizeof(float));
    use_vbo(vbo);

    glDrawArrays(GL_TRIANGLES, 0, packet->num_sides * VERTS_PER_SIDE);
}

void update_chunk_packet_at(int x, int z) {
    chunk_coord coord = {x, z};
    chunk_packet* packet = chunk_packet_map_get(&packets, coord);
    if (packet == NULL) {
        assert(false && "Packet does not exist");
    }

    free(packet->side_data);
    chunk_packet_map_remove(&packets, coord);

    chunk* c = get_chunk(x, z);
    chunk* adj_chunks[4] = {
        get_chunk(x + 1, z),
        get_chunk(x - 1, z),
        get_chunk(x, z - 1),
        get_chunk(x, z + 1)
    };
    int packet_side_count = 0;
    float* packet_sides = malloc(INITIAL_VBO_SIZE * sizeof(float));
    assert(packet_sides != NULL && "Failed to allocate memory for packet sides");
    pack_chunk(c, adj_chunks, &packet_sides, &packet_side_count);

    packet = malloc(sizeof(chunk_packet));
    assert(packet != NULL && "Failed to allocate memory for packet");

    packet->side_data = packet_sides;
    packet->num_sides = packet_side_count;
    chunk_packet_map_insert(&packets, coord, *packet);
}

void render(camera cam, shader_program program) {
    mat4 view, proj;
    get_view_matrix(cam, &view);
    uint view_loc = glGetUniformLocation(program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
    
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_atlas.id);
    uint atlas_loc = glGetUniformLocation(program.id, "atlas");
    glUniform1i(atlas_loc, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    int player_chunk_x = (int)(cam.position[0] / CHUNK_SIZE);
    int player_chunk_z = (int)(cam.position[2] / CHUNK_SIZE);

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = (int)(cam.position[0] / CHUNK_SIZE) - CHUNK_RENDER_DISTANCE + i;
            int z = (int)(cam.position[2] / CHUNK_SIZE) - CHUNK_RENDER_DISTANCE + j;

            chunk_coord coord = {x, z};
            chunk_packet* packet = chunk_packet_map_get(&packets, coord);
            if (packet == NULL) {
                chunk* c = get_chunk(x, z);
                chunk* adj_chunks[4] = {
                    get_chunk(x + 1, z),
                    get_chunk(x - 1, z),
                    get_chunk(x, z - 1),
                    get_chunk(x, z + 1)
                };
                int packet_side_count = 0;
                float* packet_sides = malloc(INITIAL_VBO_SIZE * sizeof(float));
                assert(packet_sides != NULL && "Failed to allocate memory for packet sides");
                pack_chunk(c, adj_chunks, &packet_sides, &packet_side_count);

                packet = malloc(sizeof(chunk_packet));
                assert(packet != NULL && "Failed to allocate memory for packet");
                packet->side_data = packet_sides;
                packet->num_sides = packet_side_count;

                chunk_packet_map_insert(&packets, coord, *packet);
            }
            
            render_packet(packet);
        }
    }
}