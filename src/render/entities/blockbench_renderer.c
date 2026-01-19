#include <blockbench_renderer.h>
#include "block_renderer.h"
#include "../../world/core/block.h"
#include "../../world/core/world.h"
#include <util.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "sun.h"
#include "util/settings.h"
#include <mesh.h>

blockbench_renderer create_blockbench_renderer(camera* cam, char* atlas_path, char* bump_path) {
    texture atlas = t_init(atlas_path, ATLAS_TEXTURE_INDEX);
    texture bump = t_init(bump_path, BUMP_TEXTURE_INDEX);
    
    VAO vao = create_vao();
    bind_vao(vao);
    VBO vertex_vbo = create_vbo(GL_DYNAMIC_DRAW);

    shader vertex_shader = create_shader("res/shaders/blockbench.vert", GL_VERTEX_SHADER);
    shader fragment_shader = create_shader("res/shaders/blockbench.frag", GL_FRAGMENT_SHADER);
    shader_program program = create_program(vertex_shader, fragment_shader);
    delete_shader(vertex_shader);
    delete_shader(fragment_shader);

    blockbench_renderer br = {
        .cam = cam,
        .program = program,
        .atlas = atlas,
        .bump = bump,
        .vao = vao,
        .vertex_vbo = vertex_vbo
    };

    return br;
}

void destroy_blockbench_renderer(blockbench_renderer br) {
    delete_vao(br.vao);
    delete_vbo(br.vertex_vbo);
    delete_program(br.program);
    t_cleanup(&(br.atlas));
    t_cleanup(&(br.bump));
}

void send_blockbench_view_matrix(blockbench_renderer* br) {
    mat4 view;
    get_view_matrix(*(br->cam), &view);
    uint view_loc = glGetUniformLocation(br->program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
}

void send_blockbench_proj_matrix(blockbench_renderer* br) {
    mat4 proj;
    get_projection_matrix(&proj, RADS(FOV), (float)WIDTH / (float)HEIGHT, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(br->program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);
}

void send_blockbench_atlas(blockbench_renderer* br) {
    glActiveTexture(GL_TEXTURE0 + br->atlas.tex_index);
    glBindTexture(GL_TEXTURE_2D, br->atlas.id);
    uint atlas_loc = glGetUniformLocation(br->program.id, "atlas");
    glUniform1i(atlas_loc, br->atlas.tex_index);

    glActiveTexture(GL_TEXTURE0 + br->bump.tex_index);
    glBindTexture(GL_TEXTURE_2D, br->bump.id);
    uint bump_loc = glGetUniformLocation(br->program.id, "bump");
    glUniform1i(bump_loc, br->bump.tex_index);

    uint atlas_size_loc = glGetUniformLocation(br->program.id, "atlasSize");
    glUniform1f(atlas_size_loc, (float)ATLAS_SIZE);
}

void send_blockbench_fog(blockbench_renderer* br) {
    uint fog_loc = glGetUniformLocation(br->program.id, "fogDistance");
    glUniform1f(fog_loc, RENDER_DISTANCE);
}

void send_blockbench_time(blockbench_renderer* br) {
    float current_time = (float)glfwGetTime();
    glUniform1f(glGetUniformLocation(br->program.id, "time"), current_time);
}

void render_blockbench_models(blockbench_renderer* br, sun* sun, FBO* shadow_map, world_mesh* packet) {
    if (packet == NULL || packet->custom_model_data == NULL || packet->num_custom_verts == 0) {
        return;
    }

    use_program(br->program);
    bind_vao(br->vao);

    // Send uniforms
    send_blockbench_view_matrix(br);
    send_blockbench_proj_matrix(br);
    send_blockbench_atlas(br);
    send_blockbench_fog(br);
    send_blockbench_time(br);
    send_sun_info(&(br->program), sun);
    send_ambient_light(&(br->program));
    send_sun_matrices(&(br->program), sun);
    send_fbo_texture(&(br->program), shadow_map, SHADOW_MAP_TEXTURE_INDEX, "shadowMap");
    send_shadow_info(&(br->program));

    // Buffer vertex data
    use_vbo(br->vertex_vbo);
    buffer_data(br->vertex_vbo, GL_DYNAMIC_DRAW, packet->custom_model_data, 
                packet->num_custom_verts * FLOATS_PER_MODEL_VERT * sizeof(float));

    // Set vertex attributes
    // Position (location 0): 3 floats at offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, FLOATS_PER_MODEL_VERT * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (location 1): 3 floats at offset 3
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, FLOATS_PER_MODEL_VERT * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV (location 2): 2 floats at offset 6
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, FLOATS_PER_MODEL_VERT * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Draw all vertices
    glDrawArrays(GL_TRIANGLES, 0, packet->num_custom_verts);

    // Disable vertex attributes
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    stop_program();
}
