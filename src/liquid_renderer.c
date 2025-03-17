#include <liquid_renderer.h>
#include <block.h>
#include <string.h>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shadow_map.h>

block_renderer create_liquid_renderer(camera* cam, char* atlas_path, char* bump_path, char* caustic_path) {
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

    shader vertex_shader = create_shader("res/shaders/liquid.vert", GL_VERTEX_SHADER);
    shader fragment_shader = create_shader("res/shaders/liquid.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vertex_shader, fragment_shader);
    delete_shader(vertex_shader);
    delete_shader(fragment_shader);

    block_renderer br = {
        .cam = cam,
        .program = program,
        .atlas = atlas,
        .bump = bump,
        .caustic = caustic,
        .vao = vao,
        .cube_vbo = cube_vbo,
        .instance_vbo = instance_vbo
    };

    return br;
}

void render_liquids(block_renderer* br, sun* sun, shadow_map* map, world_mesh* packet) {
    use_program(br->program);
    bind_vao(br->vao);

    send_view_matrix(br);
    send_proj_matrix(br);
    send_atlas(br);
    send_fog(br);
    send_water_info(br);
    send_time(br);
    send_sun_info(&(br->program), sun);
    send_shadow_texture(&(br->program), map);
    send_sun_matrices(&(br->program), sun);
    send_ambient_light(&(br->program));

    send_cube_vbo(br->vao, br->cube_vbo);

    if (packet != NULL) {
        render_sides(br,
            packet->liquid_data,
            packet->num_liquid_sides);
    }
}