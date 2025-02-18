#include <render.h>
#include <block.h>
#include <vbo.h>
#include <vao.h>
#include <texture.h>

VAO vao;
VBO vbo;
shader_program program;

void r_init(shader_program* program) {
    glEnable(GL_DEPTH_TEST);

    // buffers
    vao = create_vao();
    bind_vao(vao);
    vbo = create_vbo(GL_STATIC_DRAW);

    // shaders and program
    shader frag_shader = create_shader("res/shaders/shader.frag", GL_FRAGMENT_SHADER);
    shader vert_shader = create_shader("res/shaders/shader.vert", GL_VERTEX_SHADER);

    *program = create_program(vert_shader, frag_shader);
    use_program(*program);

    delete_shader(frag_shader);
    delete_shader(vert_shader);

    // texture
    t_init();
}

void r_cleanup() {
    delete_vao(vao);
    delete_vbo(vbo);
    delete_program(program);
}

void render_cube() {

}

void render(camera cam, shader_program program) {

    bind_vao(vao);
    buffer_data(vbo, GL_STATIC_DRAW, CUBE_VERTICES, sizeof(CUBE_VERTICES));
    add_attrib(&vbo, 0, 3, 0, 5 * sizeof(float));
    add_attrib(&vbo, 1, 2, 3 * sizeof(float), 5 * sizeof(float));
    use_vbo(vbo);

    mat4 view, proj;
    get_view_matrix(cam, &view);
    uint view_loc = glGetUniformLocation(program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
    
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    uint proj_loc = glGetUniformLocation(program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_atlas.id);
    uint atlas_loc = glGetUniformLocation(program.id, "atlas");
    glUniform1i(atlas_loc, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}