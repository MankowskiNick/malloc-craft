#include <block_renderer.h>
// #include <mesh.h>
#include <sort.h>
#include <block.h>
#include <world.h>
#include <glad/glad.h>


block_renderer create_block_renderer(camera* cam, char* atlas_path) {
    texture atlas = t_init(atlas_path, 0);
    
    camera_cache cam_cache = {
        .x = cam->position[0],
        .z = cam->position[2],
    };

    VAO vao = create_vao();
    bind_vao(vao);
    VBO cube_vbo = create_vbo(GL_STATIC_DRAW);
    VBO instance_vbo = create_vbo(GL_STATIC_DRAW);

    shader vertex_shader = create_shader("res/shaders/world.vert", GL_VERTEX_SHADER);
    shader fragment_shader = create_shader("res/shaders/world.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vertex_shader, fragment_shader);
    delete_shader(vertex_shader);
    delete_shader(fragment_shader);

    block_renderer sr = {
        .cam = cam,
        .program = program,
        .atlas = atlas,
        .vao = vao,
        .cube_vbo = cube_vbo,
        .instance_vbo = instance_vbo
    };

    return sr;
}

void destroy_block_renderer(block_renderer sr) {
    delete_vao(sr.vao);
    delete_vbo(sr.cube_vbo);
    delete_vbo(sr.instance_vbo);
    delete_program(sr.program);
    t_cleanup(&(sr.atlas));
}

void send_view_matrix(block_renderer* sr) {
    mat4 view;
    get_view_matrix(*(sr->cam), &view);
    uint view_loc = glGetUniformLocation(sr->program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
}

void send_proj_matrix(block_renderer* sr) {
    mat4 proj;
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(sr->program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);
}

void send_atlas(block_renderer* sr) {
    glActiveTexture(GL_TEXTURE0 + sr->atlas.tex_index);
    glBindTexture(GL_TEXTURE_2D, sr->atlas.id);
    uint atlas_loc = glGetUniformLocation(sr->program.id, "atlas");
    glUniform1i(atlas_loc, sr->atlas.tex_index);

    uint atlas_size_loc = glGetUniformLocation(sr->program.id, "atlasSize");
    glUniform1f(atlas_size_loc, (float)ATLAS_SIZE);
}

void send_fog(block_renderer* sr) {
    uint fog_loc = glGetUniformLocation(sr->program.id, "fogDistance");
    glUniform1f(fog_loc, RENDER_DISTANCE);
}

void render_sides(block_renderer* sr, int* side_data, int num_sides) {
    bind_vao(sr->vao);
    buffer_data(sr->instance_vbo, GL_STATIC_DRAW, side_data, num_sides * VBO_WIDTH * sizeof(int));
    i_add_attrib(&(sr->instance_vbo), 1, 3, 0 * sizeof(int), VBO_WIDTH * sizeof(int)); // position
    i_add_attrib(&(sr->instance_vbo), 2, 2, 3 * sizeof(int), VBO_WIDTH * sizeof(int)); // atlas coords
    i_add_attrib(&(sr->instance_vbo), 3, 1, 5 * sizeof(int), VBO_WIDTH * sizeof(int)); // side
    use_vbo(sr->instance_vbo);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
        
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, num_sides);
}

void render_solids(block_renderer* sr, chunk_mesh** packet, int num_packets) {
    use_program(sr->program);
    bind_vao(sr->vao);

    send_view_matrix(sr);
    send_proj_matrix(sr);
    send_atlas(sr);
    send_fog(sr);

    send_cube_vbo(sr->vao, sr->cube_vbo);

    for (int i = 0; i < num_packets; i++) {
        if (packet[i] == NULL) {
            continue;
        }
        render_sides(sr,
            packet[i]->opaque_data,
            packet[i]->num_opaque_sides);
    }

    stop_program();
}

void render_transparent(block_renderer* sr, chunk_mesh** packet, int num_packets) {
    use_program(sr->program);
    bind_vao(sr->vao);

    send_view_matrix(sr);
    send_proj_matrix(sr);
    send_atlas(sr);
    send_fog(sr);

    send_cube_vbo(sr->vao, sr->cube_vbo);

    for (int i = 0; i < num_packets; i++) {
        if (packet[i] == NULL) {
            continue;
        }
        render_sides(sr,
            packet[i]->transparent_data,
            packet[i]->num_transparent_sides);
    }

    stop_program();
}