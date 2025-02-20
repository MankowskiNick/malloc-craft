#include <render.h>
#include <block.h>
#include <vbo.h>
#include <vao.h>
#include <texture.h>
#include <block_types.h>
#include <chunk.h>

#include <settings.h>

#define VBO_WIDTH 7
#define VERTS_PER_SIDE 6
#define SIDE_OFFSET VERTS_PER_SIDE * VBO_WIDTH

VAO vao;
VBO vbo;
shader_program program;

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
}

void r_cleanup() {
    delete_vao(vao);
    delete_vbo(vbo);
    delete_program(program);
    t_cleanup();
}

void render_side(int x_0, int y_0, int z_0, uint side, block_type* type, float* side_data) {
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
        side_data[side_data_offset + 5] = type->face_atlas_coords[side][0];
        side_data[side_data_offset + 6] = type->face_atlas_coords[side][1];
    }
}

int get_side_visible(uint side, block_type* blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE], int x, int y, int z) {
    uint visible = 0;
    switch(side) {
        case (int)TOP:
            if (y + 1 >= CHUNK_HEIGHT) {
                visible = 1;
            }
            else {
                visible = blocks[x][y + 1][z] == NULL;
            }
            break;
        case (int)BOTTOM:
            if (y - 1 < 0) {
                visible = 1;
            }
            else {
                visible = blocks[x][y - 1][z] == NULL;
            }
            break;
        case (int)BACK:
            if (x - 1 < 0) {
                visible = 1;
            }
            else {
                visible = blocks[x - 1][y][z] == NULL;
            }
            break;
        case (int)FRONT:
            if (x + 1 >= CHUNK_SIZE) {
                visible = 1;
            }
            else {
                visible = blocks[x + 1][y][z] == NULL;
            }
            break;
        case (int)LEFT:
            if (z - 1 < 0) {
                visible = 1;
            }
            else {
                visible = blocks[x][y][z - 1] == NULL;
            }
            break;
        case (int)RIGHT:
            if (z + 1 >= CHUNK_SIZE) {
                visible = 1;
            }
            else {
                visible = blocks[x][y][z + 1] == NULL;
            }
            break;
        default:
            visible = 1;
    }
    return visible;
}

void render_block(
    float** side_data, 
    int* num_sides, 
    int x, int y, int z,
    block_type* blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE]
) {
    for (int side = 0; side < 6; side++) {
        if (!get_side_visible(side, blocks, x, y, z)) {
            continue;
        }

        int new_side_count = (*num_sides) + 1;
        float* tmp = realloc(*side_data, new_side_count * SIDE_OFFSET * sizeof(float));
        assert(tmp != NULL && "Failed to allocate memory for side data");
        *side_data = tmp;
        block_type* type = blocks[x][y][z];
        render_side(
            x, y, z, 
            side, 
            type,
            (*side_data) + (*num_sides) * SIDE_OFFSET
        );
        (*num_sides)++;
    }
}

void render_chunk(chunk* c) {
    float* side_data = malloc(SIDE_OFFSET * sizeof(float));
    int num_sides = 0;
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (c->blocks[i][k][j] == NULL) {
                    continue;
                }
                render_block(&side_data, &num_sides, i, k, j, c->blocks);
            }
        }
    }

    bind_vao(vao);
    buffer_data(vbo, GL_STATIC_DRAW, side_data, num_sides * SIDE_OFFSET * sizeof(float));
    add_attrib(&vbo, 0, 3, 0, VBO_WIDTH * sizeof(float));
    add_attrib(&vbo, 1, 2, 3 * sizeof(float), VBO_WIDTH * sizeof(float));
    add_attrib(&vbo, 2, 2, 5 * sizeof(float), VBO_WIDTH * sizeof(float));
    use_vbo(vbo);

    glDrawArrays(GL_TRIANGLES, 0, num_sides * VERTS_PER_SIDE);

    free(side_data);
}

void render(camera cam, shader_program program) {
    mat4 view, proj;
    get_view_matrix(cam, &view);
    uint view_loc = glGetUniformLocation(program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
    
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    uint proj_loc = glGetUniformLocation(program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_atlas.id);
    uint atlas_loc = glGetUniformLocation(program.id, "atlas");
    glUniform1i(atlas_loc, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    chunk c = {
        .pos = {0, 0, 0},
    };
    
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (k > 8) {
                    c.blocks[i][k][j] = NULL;  // Air block above level 8
                }
                else if (k == 8) {
                    c.blocks[i][k][j] = &TYPES[1];  // Dirt block below level 8
                }
                else {
                    c.blocks[i][k][j] = &TYPES[2];  // Grass block below level 8
                }
            }
        }
    }
    render_chunk(&c);
}