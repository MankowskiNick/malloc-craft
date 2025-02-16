#include <stdio.h>
#include <window.h>
#include <shader.h>


int main() {
    GLFWwindow* window = create_window("c-craft", 800, 600);
    if (!window || !load_gl()) {
        return -1;
    }

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    // Vertex Array Object
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Vertex Buffer Object
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);  
    
    shader* frag_shader = create_shader("shader/shader.frag", GL_FRAGMENT_SHADER);
    shader* vert_shader = create_shader("shader/shader.vert", GL_VERTEX_SHADER);

    shader_program* program = create_program(vert_shader, frag_shader);
    glUseProgram(program->id);

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