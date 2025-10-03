#include <fbo.h>

#include <glad/glad.h>
#include <vao.h>
#include <vbo.h>
#include <block.h>

void FBO_cleanup(FBO* map) {
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

void send_fbo_texture(shader_program* program, FBO* map, uint texture_index, char* uniform_name) {
    glActiveTexture(GL_TEXTURE0 + texture_index);
    glBindTexture(GL_TEXTURE_2D, map->texture);
    uint shadow_loc = glGetUniformLocation(program->id, uniform_name);
    glUniform1i(shadow_loc, texture_index);
}

void render_depth(FBO* map, int* side_data, int num_sides) {
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

void FBO_render(FBO* map, sun* s, world_mesh* packet) {
    // Save current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glBindFramebuffer(GL_FRAMEBUFFER, map->fbo);
    glViewport(0, 0, map->width, map->height);
    glClear(GL_DEPTH_BUFFER_BIT);

    use_program(map->program);

    send_sun_matrices(&(map->program), s);

    // send cube vbo
    send_cube_vbo(map->vao, map->cube_vbo);

    if (packet != NULL) {
        render_depth(map,
            packet->opaque_data,
            packet->num_opaque_sides);
        render_depth(map,
            packet->transparent_data,
            packet->num_transparent_sides);
    }


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Restore the viewport to what it was before
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

}