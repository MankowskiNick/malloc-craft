#include <block_renderer.h>
// #include <mesh.h>
#include <sort.h>
#include <block.h>
#include <world.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


block_renderer create_block_renderer(camera* cam, char* atlas_path, char* caustic_path) {
    texture atlas = t_init(atlas_path, ATLAS_TEXTURE_INDEX);
    texture caustic = t_init(caustic_path, CAUSTIC_TEXTURE_INDEX);
    
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
        .caustic = caustic,
        .vao = vao,
        .cube_vbo = cube_vbo,
        .instance_vbo = instance_vbo
    };

    return sr;
}

void destroy_block_renderer(block_renderer br) {
    delete_vao(br.vao);
    delete_vbo(br.cube_vbo);
    delete_vbo(br.instance_vbo);
    delete_program(br.program);
    t_cleanup(&(br.atlas));
}

void send_view_matrix(block_renderer* br) {
    mat4 view;
    get_view_matrix(*(br->cam), &view);
    uint view_loc = glGetUniformLocation(br->program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
}

void send_proj_matrix(block_renderer* br) {
    mat4 proj;
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(br->program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);
}

void send_atlas(block_renderer* br) {
    glActiveTexture(GL_TEXTURE0 + br->atlas.tex_index);
    glBindTexture(GL_TEXTURE_2D, br->atlas.id);
    uint atlas_loc = glGetUniformLocation(br->program.id, "atlas");
    glUniform1i(atlas_loc, br->atlas.tex_index);

    uint atlas_size_loc = glGetUniformLocation(br->program.id, "atlasSize");
    glUniform1f(atlas_size_loc, (float)ATLAS_SIZE);
}


void send_water_info(block_renderer* br) {
    glUniform1f(glGetUniformLocation(br->program.id, "waterOffset"), WATER_OFFSET);
    glUniform1f(glGetUniformLocation(br->program.id, "waterLevel"), (float)WORLDGEN_WATER_LEVEL);

    glActiveTexture(GL_TEXTURE0 + br->caustic.tex_index);
    glBindTexture(GL_TEXTURE_2D, br->caustic.id);
    uint caustic_loc = glGetUniformLocation(br->program.id, "caustic");
    glUniform1i(caustic_loc, br->caustic.tex_index);

    uint water_dist_loc = glGetUniformLocation(br->program.id, "waterDistance");
    glUniform1f(water_dist_loc, WATER_DISTANCE);
}

void send_fog(block_renderer* br) {
    uint fog_loc = glGetUniformLocation(br->program.id, "fogDistance");
    glUniform1f(fog_loc, RENDER_DISTANCE);
}

void send_time(block_renderer* br) {
    float current_time = (float)glfwGetTime();
    glUniform1f(glGetUniformLocation(br->program.id, "time"), current_time);
}

void send_ambient_light(shader_program* p) {
    glUniform3f(glGetUniformLocation(p->id, "ambientLight"), AMBIENT_R_INTENSITY, AMBIENT_G_INTENSITY, AMBIENT_B_INTENSITY);
}

void render_sides(block_renderer* br, int* side_data, int num_sides) {
    bind_vao(br->vao);
    buffer_data(br->instance_vbo, GL_STATIC_DRAW, side_data, num_sides * VBO_WIDTH * sizeof(int));
    i_add_attrib(&(br->instance_vbo), 1, 3, 0 * sizeof(int), VBO_WIDTH * sizeof(int)); // position
    i_add_attrib(&(br->instance_vbo), 2, 2, 3 * sizeof(int), VBO_WIDTH * sizeof(int)); // atlas coords
    i_add_attrib(&(br->instance_vbo), 3, 1, 5 * sizeof(int), VBO_WIDTH * sizeof(int)); // side
    i_add_attrib(&(br->instance_vbo), 4, 1, 6 * sizeof(int), VBO_WIDTH * sizeof(int)); // underwater
    use_vbo(br->instance_vbo);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
        
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, num_sides);
}

void render_solids(block_renderer* br, sun* sun, chunk_mesh** packet, int num_packets) {
    use_program(br->program);
    bind_vao(br->vao);

    send_view_matrix(br);
    send_proj_matrix(br);
    send_atlas(br);
    send_water_info(br);
    send_fog(br);
    send_time(br);
    send_sun_info(&(br->program), sun);
    send_ambient_light(&(br->program));

    send_cube_vbo(br->vao, br->cube_vbo);

    for (int i = 0; i < num_packets; i++) {
        if (packet[i] == NULL) {
            continue;
        }
        render_sides(br,
            packet[i]->opaque_data,
            packet[i]->num_opaque_sides);
    }

    stop_program();
}

void render_transparent(block_renderer* br, sun* sun, chunk_mesh** packet, int num_packets) {
    use_program(br->program);
    bind_vao(br->vao);

    send_view_matrix(br);
    send_proj_matrix(br);
    send_atlas(br);
    send_fog(br);
    send_time(br);
    send_sun_info(&(br->program), sun);
    send_ambient_light(&(br->program));

    send_cube_vbo(br->vao, br->cube_vbo);

    for (int i = 0; i < num_packets; i++) {
        if (packet[i] == NULL) {
            continue;
        }
        render_sides(br,
            packet[i]->transparent_data,
            packet[i]->num_transparent_sides);
    }

    stop_program();
}