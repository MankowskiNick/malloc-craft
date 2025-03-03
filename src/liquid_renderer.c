#include <liquid_renderer.h>
#include <block.h>
#include <string.h>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

block_renderer create_liquid_renderer(camera* cam, char* atlas_path) {
    texture atlas = t_init(atlas_path, 0);
    
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
        .vao = vao,
        .cube_vbo = cube_vbo,
        .instance_vbo = instance_vbo
    };

    return br;
}

void send_water_info(block_renderer* br) {
    glUniform1f(glGetUniformLocation(br->program.id, "waterOffset"), WATER_OFFSET);
    glUniform1f(glGetUniformLocation(br->program.id, "waterLevel"), (float)WORLDGEN_WATER_LEVEL);

    float current_time = (float)glfwGetTime();
    glUniform1f(glGetUniformLocation(br->program.id, "time"), current_time);
}

void render_liquids(block_renderer* br, chunk_mesh** packet, int num_packets) {
    use_program(br->program);
    bind_vao(br->vao);

    send_view_matrix(br);
    send_proj_matrix(br);
    send_atlas(br);
    send_fog(br);
    send_water_info(br);

    send_cube_vbo(br->vao, br->cube_vbo);

    for (int i = 0; i < num_packets; i++) {
        if (packet[i] == NULL) {
            continue;
        }
        render_sides(br,
            packet[i]->liquid_data,
            packet[i]->num_liquid_sides);
    }
}