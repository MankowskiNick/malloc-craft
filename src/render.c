#include <render.h>
#include <block.h>
#include <vbo.h>
#include <vao.h>
#include <texture.h>

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

void set_block(block* b, uint id, int x, int y, int z, float face_atlas_coords[6][2]) {
    b->id = id;
    b->pos[0] = x;
    b->pos[1] = y;
    b->pos[2] = z;   
    for (int i = 0; i < 6; i++) {
        b->face_atlas_coords[i][0] = face_atlas_coords[i][0];
        b->face_atlas_coords[i][1] = face_atlas_coords[i][1];
    }
}

void render_block(block b, float* block_data) {
    // copy side info to vertices array
    for (int i = 0; i < 36; i ++) {
        
        // unpack vertex
        float x = CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 0];
        float y = CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 1];
        float z = CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 2];
        float tx = CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 3];
        float ty = CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 4];
        int side = (int)CUBE_VERTICES[i * CUBE_VERTICES_WIDTH + 5];
        
        // copy vertex info into vertices array
        int offset = i * 7;
        block_data[offset + 0] = x + (float)b.pos[0];
        block_data[offset + 1] = y + (float)b.pos[1];
        block_data[offset + 2] = z + (float)b.pos[2];
        block_data[offset + 3] = tx;
        block_data[offset + 4] = ty;
        block_data[offset + 5] = b.face_atlas_coords[side][0];
        block_data[offset + 6] = b.face_atlas_coords[side][1];
    }
}

void render_chunk() {
    block blocks[CHUNK_SIZE][CHUNK_SIZE];
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            float face_atlas_coords[6][2] = {
                {1.0f / 32.0f, 0.0f},
                {1.0f / 32.0f, 0.0f},
                {0.0f, 0.0f},
                {2.0f / 32.0f, 0.0f},
                {1.0f / 32.0f, 0.0f},
                {1.0f / 32.0f, 0.0f}
            };
            set_block(&blocks[i][j], 1, i, 0, j, face_atlas_coords);
        }
    }

    float block_data[CHUNK_SIZE * CHUNK_SIZE * 36 * 7];
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            render_block(
                blocks[i][j], 
                &block_data[(i * CHUNK_SIZE + j) * 36 * 7]
            );
        }
    }

    bind_vao(vao);
    buffer_data(vbo, GL_STATIC_DRAW, block_data, sizeof(block_data));
    add_attrib(&vbo, 0, 3, 0, 7 * sizeof(float));
    add_attrib(&vbo, 1, 2, 3 * sizeof(float), 7 * sizeof(float));
    add_attrib(&vbo, 2, 2, 5 * sizeof(float), 7 * sizeof(float));
    use_vbo(vbo);

    glDrawArrays(GL_TRIANGLES, 0, CHUNK_SIZE * CHUNK_SIZE * 36);
    // wireframe mode

    // glEnableVertexAttribArray(0);
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
    render_chunk();

    // glDrawArrays(GL_TRIANGLES, 0, 36);
}