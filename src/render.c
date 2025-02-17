#include <render.h>
#include <vbo.h>
#include <vao.h>
#include <shader.h>
#include <block.h>

VAO vao;
VBO vbo;
shader_program program;

void r_init() {
    // buffers
    vao = create_vao();
    bind_vao(vao);
    vbo = create_vbo(GL_STATIC_DRAW);

    // shaders and program
    shader frag_shader = create_shader("res/shaders/shader.frag", GL_FRAGMENT_SHADER);
    shader vert_shader = create_shader("res/shaders/shader.vert", GL_VERTEX_SHADER);

    shader_program program = create_program(vert_shader, frag_shader);
    use_program(program);

    delete_shader(frag_shader);
    delete_shader(vert_shader);
}

void r_cleanup() {
    delete_vao(vao);
    delete_vbo(vbo);
    delete_program(program);
}

void render_cube() {

}

void render() {
    bind_vao(vao);
    buffer_data(vbo, GL_STATIC_DRAW, CUBE_VERTICES, sizeof(CUBE_VERTICES));
    add_attrib(&vbo, 0, 3, 0, 5 * sizeof(float));
    add_attrib(&vbo, 1, 2, 3 * sizeof(float), 5 * sizeof(float));
    use_vbo(vbo);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}