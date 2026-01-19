#include "foliage_renderer.h"
// #include "mesh.h"
#include "util/sort.h"
#include "../../world/core/block.h"
#include "../../world/core/world.h"
#include <util.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void render_foliage_sides(block_renderer* br, int* foliage_data, int num_foliage_sides) {
    bind_vao(br->vao);
    buffer_data(br->instance_vbo, GL_STATIC_DRAW, foliage_data, num_foliage_sides * VBO_WIDTH * sizeof(int));
    i_add_attrib(&(br->instance_vbo), 1, 3, 0 * sizeof(int), VBO_WIDTH * sizeof(int)); // position
    i_add_attrib(&(br->instance_vbo), 2, 2, 3 * sizeof(int), VBO_WIDTH * sizeof(int)); // atlas coords
    i_add_attrib(&(br->instance_vbo), 3, 1, 5 * sizeof(int), VBO_WIDTH * sizeof(int)); // side
    use_vbo(br->instance_vbo);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
        
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, num_foliage_sides);
}

block_renderer create_foliage_renderer(camera* cam, char* atlas_path, char* bump_path, char* caustic_path) {
    texture atlas = t_init(atlas_path, ATLAS_TEXTURE_INDEX);
    texture bump = t_init(bump_path, BUMP_TEXTURE_INDEX);
    texture caustic = t_init(caustic_path, CAUSTIC_TEXTURE_INDEX);
    
    camera_cache cam_cache = {
        .x = cam->position[0],
        .z = cam->position[2],
    };

    VAO vao = create_vao();
    bind_vao(vao);
    VBO cube_vbo = create_vbo(GL_STATIC_DRAW);
    VBO instance_vbo = create_vbo(GL_STATIC_DRAW);

    shader vertex_shader = create_shader(FOLIAGE_VERTEX_SHADER, GL_VERTEX_SHADER);
    shader fragment_shader = create_shader(FOLIAGE_FRAGMENT_SHADER, GL_FRAGMENT_SHADER);
    shader_program program = create_program(vertex_shader, fragment_shader);
    delete_shader(vertex_shader);
    delete_shader(fragment_shader);

    block_renderer sr = {
        .cam = cam,
        .program = program,
        .atlas = atlas,
        .bump = bump,
        .caustic = caustic,
        .vao = vao,
        .cube_vbo = cube_vbo,
        .instance_vbo = instance_vbo
    };

    return sr;
}

void destroy_foliage_renderer(block_renderer br) {
    delete_vao(br.vao);
    delete_vbo(br.cube_vbo);
    delete_vbo(br.instance_vbo);
    delete_program(br.program);
    t_cleanup(&(br.atlas));
}

void render_foliage(block_renderer* br, sun* sun, FBO* map, world_mesh* packet) {
    use_program(br->program);
    bind_vao(br->vao);

    send_view_matrix(br);
    send_proj_matrix(br);
    send_atlas(br);
    send_fog(br);
    send_time(br);
    send_sun_info(&(br->program), sun);
    send_sun_matrices(&(br->program), sun);
    send_fbo_texture(&(br->program), map, SHADOW_MAP_TEXTURE_INDEX, "shadowMap");
    send_ambient_light(&(br->program));
    send_shadow_info(&(br->program));

    send_cube_vbo(br->vao, br->cube_vbo);

    if (packet != NULL) {
        render_foliage_sides(br,
            packet->foliage_data,
            packet->num_foliage_sides);
    }

    stop_program();
}
