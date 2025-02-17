#include <stdio.h>
#include <window.h>
#include <shader.h>
#include <vao.h>
#include <vbo.h>
#include <util.h>


int main() {
    GLFWwindow* window = create_window("malloc-craft", 800, 600);
    if (!window || !load_gl()) {
        return -1;
    }

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    // Vertex Array Object
    // VAO vao = create_vao();
    // bind_vao(vao);
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Vertex Buffer Object
    VBO vbo = create_vbo(0, 3, 3 * sizeof(float), 0);
    buffer_data(vbo, GL_STATIC_DRAW, vertices, sizeof(vertices));
    use_vbo(vbo);

    shader* frag_shader = create_shader("shader/shader.frag", GL_FRAGMENT_SHADER);
    shader* vert_shader = create_shader("shader/shader.vert", GL_VERTEX_SHADER);

    shader_program* program = create_program(vert_shader, frag_shader);
    use_program(program);

    delete_shader(frag_shader);
    delete_shader(vert_shader);


    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        // Render OpenGL here
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete_program(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}