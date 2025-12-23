#include <liquid_renderer.h>
#include <block.h>
#include <string.h>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fbo.h>

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

void render_liquids(block_renderer* br, sun* sun, FBO* shadow_map, FBO* reflection_map, world_mesh* packet) {
    use_program(br->program);
    bind_vao(br->vao);

    // Enable blending for transparent liquid volumes
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable depth writing so water chunks properly occlude each other
    glDepthMask(GL_TRUE);

    // Use LESS to prevent z-fighting at chunk boundaries
    glDepthFunc(GL_LESS);

    // Only use polygon offset when underwater to avoid chunk seams when viewing from above
    // When looking at water from above, proper sorting (front-to-back) should handle blending
    if (br->cam->position[1] <= (float)(WORLDGEN_WATER_LEVEL + 1)) {
        // Enable polygon offset to allow water to render over coplanar solid blocks
        // This creates a small depth separation that allows caustics to show
        // Negative offset pulls water toward camera in depth space
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
    }

    send_view_matrix(br);
    send_proj_matrix(br);
    send_atlas(br);
    send_fog(br);
    send_water_info(br);
    send_caustic_texture(br);
    send_time(br);
    send_sun_info(&(br->program), sun);
    send_fbo_texture(&(br->program), shadow_map, SHADOW_MAP_TEXTURE_INDEX, "shadowMap");
    send_fbo_texture(&(br->program), reflection_map, REFLECTION_MAP_TEXTURE_INDEX, "reflectionMap");
    send_sun_matrices(&(br->program), sun);
    send_ambient_light(&(br->program));
    send_shadow_info(&(br->program));

    send_cube_vbo(br->vao, br->cube_vbo);

    if (packet != NULL) {
        render_sides(br,
            packet->liquid_data,
            packet->num_liquid_sides);
    }

    // Disable polygon offset
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Restore depth mask and depth function
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}
