#include <shadow_map.h>

#include <glad/glad.h>
#include <vao.h>
#include <vbo.h>
#include <block.h>

shadow_map create_shadow_map(uint width, uint height) {

    // create fbo
    uint fbo;
    glGenFramebuffers(1, &fbo);

    // create program
    shader vert_shader = create_shader("res/shaders/shadow.vert", GL_VERTEX_SHADER);
    shader frag_shader = create_shader("res/shaders/shadow.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vert_shader, frag_shader);
    delete_shader(vert_shader);
    delete_shader(frag_shader);

    // create depth buffer
    uint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // create vao
    VAO vao = create_vao();
    bind_vao(vao);

    // create cube vbo
    VBO cube_vbo = create_vbo(GL_STATIC_DRAW);
    VBO instance_vbo = create_vbo(GL_STATIC_DRAW);

    // bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

    // disable writes to color buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // check framebuffer
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Error: Framebuffer is not complete\n");
    }

    // unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shadow_map map = {
        .width = width,
        .height = height,
        .fbo = fbo,
        .texture = texture,
        .program = program,
        .vao = vao,
        .cube_vbo = cube_vbo,
        .instance_vbo = instance_vbo
    };

    return map;
}

void shadow_map_cleanup(shadow_map* map) {
    glDeleteFramebuffers(1, &map->fbo);
    glDeleteTextures(1, &map->texture);
}

void get_sun_view_matrix(vec3 pos, vec3 player_pos, mat4* view) {
    vec3 sun_pos = {pos[0] + player_pos[0], pos[1] + player_pos[1], pos[2] + player_pos[2]};
    glm_lookat(sun_pos, 
        player_pos,
        (vec3){0.0f, 1.0f, 0.0f},
        *view);
}

void get_sun_proj_matrix(mat4* proj, sun* s) {
    float x = s->cam->position[0];
    float y = s->cam->position[1];
    float z = s->cam->position[2];
    glm_ortho(-SHADOW_RENDER_DIST, SHADOW_RENDER_DIST, 
        -SHADOW_RENDER_DIST, SHADOW_RENDER_DIST, 
        -SHADOW_RENDER_DIST, SHADOW_RENDER_DIST, 
        *proj);
}

void send_sun_matrices(shader_program* program, sun* sun) {
    mat4 view;
    get_sun_view_matrix(
        (vec3){sun->x, sun->y, sun->z}, 
        (vec3){sun->cam->position[0], sun->cam->position[1], sun->cam->position[2]},
        &view);
    uint view_loc = glGetUniformLocation(program->id, "sunView");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);

    mat4 proj;
    get_sun_proj_matrix(&proj, sun);
    uint proj_loc = glGetUniformLocation(program->id, "sunProj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);
}

void send_shadow_texture(shader_program* program, shadow_map* map) {
    glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, map->texture);
    uint shadow_loc = glGetUniformLocation(program->id, "shadowMap");
    glUniform1i(shadow_loc, SHADOW_MAP_TEXTURE_INDEX);
}

void render_depth(shadow_map* map, int* side_data, int num_sides) {
    use_program(map->program);
    bind_vao(map->vao);
    buffer_data(map->instance_vbo, GL_STATIC_DRAW, side_data, num_sides * VBO_WIDTH * sizeof(int));
    i_add_attrib(&(map->instance_vbo), 1, 3, 0 * sizeof(int), VBO_WIDTH * sizeof(int)); // position
    i_add_attrib(&(map->instance_vbo), 2, 2, 3 * sizeof(int), VBO_WIDTH * sizeof(int)); // atlas coords
    i_add_attrib(&(map->instance_vbo), 3, 1, 5 * sizeof(int), VBO_WIDTH * sizeof(int)); // side
    i_add_attrib(&(map->instance_vbo), 4, 1, 6 * sizeof(int), VBO_WIDTH * sizeof(int)); // underwater
    use_vbo(map->instance_vbo);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, num_sides);

}

void shadow_map_render(shadow_map* map, sun* s, chunk_mesh** packet, int num_packets) {
    // Save current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glBindFramebuffer(GL_FRAMEBUFFER, map->fbo);
    glViewport(0, 0, map->width, map->height);
    glClear(GL_DEPTH_BUFFER_BIT);

    use_program(map->program);
    // bind_vao(map->vao);

    send_sun_matrices(&(map->program), s);

    // send cube vbo
    send_cube_vbo(map->vao, map->cube_vbo);

    // render scene here
    for (int i = 0; i < num_packets; i++) {
        if (packet[i] == NULL) {
            continue;
        }
        render_depth(map,
            packet[i]->opaque_data,
            packet[i]->num_opaque_sides);
        render_depth(map,
            packet[i]->transparent_data,
            packet[i]->num_transparent_sides);
    }


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Restore the viewport to what it was before
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

}