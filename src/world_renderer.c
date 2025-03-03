#include <world_renderer.h>
#include <mesh.h>
#include <sort.h>
#include <block.h>
#include <world.h>
#include <glad/glad.h>


world_renderer create_world_renderer(camera* cam, char* atlas_path) {
    // r_cam_ref = cam;
    
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

    w_init();
    m_init(cam);

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = cam_cache.x - CHUNK_RENDER_DISTANCE + i;
            int z = cam_cache.z - CHUNK_RENDER_DISTANCE + j;
            get_chunk_mesh(x, z);
            load_chunk();
        }
    }

    world_renderer wr = {
        .cam = cam,
        // .cam_cache = cam_cache,
        .program = program,
        .atlas = atlas,
        .vao = vao,
        .cube_vbo = cube_vbo,
        .instance_vbo = instance_vbo
    };

    return wr;
}

void destroy_world_renderer(world_renderer wr) {
    delete_vao(wr.vao);
    delete_vbo(wr.cube_vbo);
    delete_vbo(wr.instance_vbo);
    delete_program(wr.program);
    t_cleanup(&(wr.atlas));
    m_cleanup();
}

void send_view_matrix(world_renderer* wr) {
    mat4 view;
    get_view_matrix(*(wr->cam), &view);
    uint view_loc = glGetUniformLocation(wr->program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
}

void send_proj_matrix(world_renderer* wr) {
    mat4 proj;
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(wr->program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);
}

void send_atlas(world_renderer* wr) {
    glActiveTexture(GL_TEXTURE0 + wr->atlas.tex_index);
    glBindTexture(GL_TEXTURE_2D, wr->atlas.id);
    uint atlas_loc = glGetUniformLocation(wr->program.id, "atlas");
    glUniform1i(atlas_loc, wr->atlas.tex_index);

    uint atlas_size_loc = glGetUniformLocation(wr->program.id, "atlasSize");
    glUniform1f(atlas_size_loc, (float)ATLAS_SIZE);
}

void send_fog(world_renderer* wr) {
    uint fog_loc = glGetUniformLocation(wr->program.id, "fogDistance");
    glUniform1f(fog_loc, RENDER_DISTANCE);
}



void render_sides(world_renderer* wr, int* side_data, int num_sides) {
    bind_vao(wr->vao);
    buffer_data(wr->instance_vbo, GL_STATIC_DRAW, side_data, num_sides * VBO_WIDTH * sizeof(int));
    i_add_attrib(&(wr->instance_vbo), 1, 3, 0 * sizeof(int), VBO_WIDTH * sizeof(int)); // position
    i_add_attrib(&(wr->instance_vbo), 2, 2, 3 * sizeof(int), VBO_WIDTH * sizeof(int)); // atlas coords
    i_add_attrib(&(wr->instance_vbo), 3, 1, 5 * sizeof(int), VBO_WIDTH * sizeof(int)); // side
    use_vbo(wr->instance_vbo);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
        
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, num_sides);
}

void render_world(world_renderer* wr, chunk_mesh** packet, int num_packets) {
    use_program(wr->program);
    bind_vao(wr->vao);

    send_view_matrix(wr);
    send_proj_matrix(wr);
    send_atlas(wr);
    send_fog(wr);

    send_cube_vbo(wr->vao, wr->cube_vbo);
    glClear(GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < num_packets; i++) {
        if (packet[i] == NULL) {
            continue;
        }
        render_sides(wr,
            packet[i]->opaque_data,
            packet[i]->num_opaque_sides);
        render_sides(wr,
            packet[i]->transparent_data,
            packet[i]->num_transparent_sides);
    }

    free(packet);
    stop_program();
}
