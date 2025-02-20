#include <render.h>
#include <block.h>
#include <vbo.h>
#include <vao.h>
#include <texture.h>
#include <block_types.h>
#include <chunk.h>

#include <settings.h>

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
}

void render_block(int x_0, int y_0, int z_0, block_type* type, float* block_data) {
    // copy side info to vertices array
    for (int i = 0; i < 36; i ++) {
        
        // unpack vertex
        float x = x_0 + CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 0];
        float y = y_0 + CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 1];
        float z = z_0 + CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 2];
        float tx = CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 3];
        float ty = CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 4];
        int side = (int)CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 5];
        
        // copy vertex info into vertices array
        int offset = i * 7;
        block_data[offset + 0] = x;
        block_data[offset + 1] = y;
        block_data[offset + 2] = z;
        block_data[offset + 3] = tx;
        block_data[offset + 4] = ty;
        block_data[offset + 5] = type->face_atlas_coords[side][0];
        block_data[offset + 6] = type->face_atlas_coords[side][1];
    }
}

void render_chunk(chunk* c) {
    float block_data[CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 36 * 7];
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (c->blocks[i][k][j] == NULL) {
                    continue;
                }
                
                render_block(
                    i, k, j, 
                    c->blocks[i][k][j], 
                    block_data + (i * CHUNK_SIZE * CHUNK_HEIGHT + k * CHUNK_SIZE + j) * 36 * 7
                );
            }
        }
    }

    bind_vao(vao);
    buffer_data(vbo, GL_STATIC_DRAW, block_data, sizeof(block_data));
    add_attrib(&vbo, 0, 3, 0, 7 * sizeof(float));
    add_attrib(&vbo, 1, 2, 3 * sizeof(float), 7 * sizeof(float));
    add_attrib(&vbo, 2, 2, 5 * sizeof(float), 7 * sizeof(float));
    use_vbo(vbo);

    glDrawArrays(GL_TRIANGLES, 0, CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 36);
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
                    c.blocks[i][k][j] = &TYPES[0];  // Air block above level 8
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